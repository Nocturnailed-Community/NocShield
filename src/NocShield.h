#ifndef NOCSHIELD_H
#define NOCSHIELD_H

#include <Arduino.h>

#ifdef ESP32
#include <WiFi.h>
#include <esp_wifi.h>
#include <DNSServer.h>
#include <WebServer.h>
#include <mbedtls/md.h>
#include <mbedtls/aes.h>
#include <esp_system.h>
#else
#error "NocShield currently only supports ESP32"
#endif

class NocShield {
public:
    NocShield();
    ~NocShield();

    // Rule-of-three fix: this class owns raw pointers (_dnsServer, _webServer).
    // The compiler-generated copy constructor/assignment would shallow-copy
    // those pointers, causing a double-free when both copies are destroyed.
    // Disable copying until proper deep-copy or shared ownership is implemented.
    NocShield(const NocShield&) = delete;
    NocShield& operator=(const NocShield&) = delete;

    void begin();

    // Offensive Operations
    // Note: Use responsibly and only on your own networks!
    bool sendDeauth(uint8_t *targetMac, uint8_t *apMac, uint8_t channel, uint16_t reason = 1);
    
    // Captive Portal (Evil Twin / Phishing)
    void startCaptivePortal(const char* ssid, const char* htmlContent);
    void handleCaptivePortal();

    // Defensive Operations
    void startPacketMonitor(wifi_promiscuous_cb_t callback);
    void stopPacketMonitor();

    // Hardware Cryptography & Security Operations
    String hashSHA256(const String& payload);
    String encryptAES(const String& plaintext, const uint8_t* key, const uint8_t* iv);
    String decryptAES(const String& ciphertextHex, const uint8_t* key, const uint8_t* iv);
    uint32_t generateRandomNumber();

private:
    bool _isMonitoring;
    DNSServer* _dnsServer;
    WebServer* _webServer;
    const char* _portalHtml;
    
    String bytesToHex(const uint8_t* bytes, size_t length);
    // BUG FIX: now returns false if `hex` is too short for `length` bytes,
    // instead of silently producing garbage output.
    bool hexToBytes(const String& hex, uint8_t* bytes, size_t length);
};

#endif // NOCSHIELD_H