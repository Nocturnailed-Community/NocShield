#include <Arduino.h>
#include "NocShield.h"

NocShield nocShield;

// 256-bit AES Key (32 bytes)
const uint8_t aesKey[32] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 
    0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 
    0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F
};

// 128-bit Initialization Vector (16 bytes)
const uint8_t aesIV[16] = {
    0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x00, 0x11, 
    0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99
};

void setup() {
    Serial.begin(115200);
    delay(2000);
    Serial.println("\n--- NocShield Hardware Cryptography Simulation ---");
    
    String secretMessage = "Confidential Sensor Data: 42.5C";
    Serial.println("Original Message: " + secretMessage);
    
    // 1. Hashing (SHA256)
    // Used to verify data integrity
    String messageHash = nocShield.hashSHA256(secretMessage);
    Serial.println("SHA-256 Hash: " + messageHash);
    
    // 2. Encryption (AES-256-CBC)
    // Used to protect data confidentiality
    String encryptedHex = nocShield.encryptAES(secretMessage, aesKey, aesIV);
    Serial.println("Encrypted (Hex): " + encryptedHex);
    
    // 3. Decryption
    String decryptedMessage = nocShield.decryptAES(encryptedHex, aesKey, aesIV);
    Serial.println("Decrypted Message: " + decryptedMessage);
    
    if (decryptedMessage == secretMessage) {
        Serial.println("[SUCCESS] Decryption matches original message!");
    } else {
        Serial.println("[ERROR] Decryption failed.");
    }
    
    // 4. True Random Number Generation
    Serial.print("Hardware Random Number: ");
    Serial.println(nocShield.generateRandomNumber());
}

void loop() {
    delay(1000);
}
