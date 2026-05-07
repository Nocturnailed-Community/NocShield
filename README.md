# NocShield

[![Arduino Lint](https://github.com/Nocturnailed-Community/NocShield/actions/workflows/lint.yml/badge.svg)](https://github.com/Nocturnailed-Community/NocShield/actions/workflows/lint.yml)

*[Baca dokumentasi dalam Bahasa Indonesia (README_ID.md)](README_ID.md)*

**NocShield** is a modular and lightweight library for Network Security simulations on ESP32. It brings the power of offensive and defensive network operations to microcontrollers, making it an excellent tool for learning and demonstrating network vulnerabilities and mitigations.

> **⚠️ DISCLAIMER**
> This library is created for **EDUCATIONAL PURPOSES ONLY**. Do not use these tools on networks or devices you do not own or have explicit permission to test. The authors and maintainers are not responsible for any misuse or damage caused by this software.

## Features

*   **Offensive Simulations:**
    *   **Deauthentication Attacks:** Send 802.11 Deauth frames to disconnect clients.
    *   **Captive Portal (Evil Twin):** Set up a fake Access Point with a phishing login page to intercept credentials.
    *   **Hash Cracking:** Utilize ESP32's hardware acceleration to simulate dictionary/brute-force attacks against SHA hashes.
*   **Defensive Simulations:**
    *   **Packet Monitoring (Sniffer):** Use Promiscuous Mode to capture and analyze raw Wi-Fi frames in the air.
    *   **Deauth Detector (IDS):** Detect deauthentication storms and trigger alarms.
*   **Secure Communications:**
    *   **Hardware Cryptography:** Fast AES encryption/decryption and SHA-256 hashing using the ESP32's built-in `mbedtls` engine.
    *   **Secure Clients:** Examples demonstrating the correct use of Root CA certificates to prevent Man-in-the-Middle (MitM) attacks.

## Installation

1.  Download this repository as a ZIP file.
2.  Open your Arduino IDE.
3.  Go to **Sketch** -> **Include Library** -> **Add .ZIP Library...**
4.  Select the downloaded ZIP file.

*Note: This library currently only supports ESP32 microcontrollers.*

## Examples

You can find several hands-on examples under **File** -> **Examples** -> **NocShield**:

*   **Offensive:**
    *   `DeauthAttack`: Simulates disconnecting a specific client from an AP.
    *   `CaptivePortal`: Creates a fake "Free Public WiFi" that captures entered credentials.
    *   `HashCracker`: Demonstrates a dictionary attack against a SHA-256 hash.
*   **Defensive:**
    *   `PacketMonitor`: Captures raw 802.11 Management frames.
    *   `DeauthDetector`: Triggers an alarm when a Deauth attack is detected.
*   **SecureComms:**
    *   `SecureClient`: Demonstrates secure HTTPS requests using Root CA validation.
    *   `HardwareCrypto`: Shows how to use hardware-accelerated AES and SHA functions.

## API Reference

### Initialization
```cpp
NocShield nocShield;
nocShield.begin();
```

### Offensive Operations
```cpp
// Send a Deauth Frame
bool success = nocShield.sendDeauth(targetMac, apMac, channel, reasonCode);

// Start an Evil Twin Captive Portal
nocShield.startCaptivePortal("Free Public WiFi", htmlContent);
nocShield.handleCaptivePortal(); // Call this in loop()
```

### Defensive Operations
```cpp
// Start capturing raw packets
nocShield.startPacketMonitor(myCallbackFunction);

// Stop capturing
nocShield.stopPacketMonitor();
```

### Hardware Cryptography
```cpp
// Generate true random number
uint32_t rng = nocShield.generateRandomNumber();

// SHA-256 Hashing
String hash = nocShield.hashSHA256("my_secret_password");

// AES-256-CBC Encryption/Decryption
String encryptedHex = nocShield.encryptAES("Data", aesKey, aesIV);
String decryptedStr = nocShield.decryptAES(encryptedHex, aesKey, aesIV);
```

## Contributing

Contributions are welcome! Please ensure you test your code and run the Arduino Lint before submitting pull requests.

## License

This project is maintained by the [Nocturnailed Community](https://github.com/Nocturnailed-Community).
