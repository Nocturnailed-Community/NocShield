#include <Arduino.h>
#include "NocShield.h"

NocShield nocShield;

// Target SHA-256 Hash to crack (This is the hash of "password123")
const String targetHash = "ef92b778bafe771e89245b89ecbc08a44a4e166c06659911881f383d4473e94f";

// A small dictionary of common passwords
const char* dictionary[] = {
    "admin",
    "admin123",
    "123456",
    "password",
    "qwerty",
    "iloveyou",
    "password123", // The correct one
    "letmein",
    "111111"
};
const int dictionarySize = sizeof(dictionary) / sizeof(dictionary[0]);

void setup() {
    Serial.begin(115200);
    delay(2000);
    Serial.println("\n--- NocShield Hash Cracker (Brute-Force) Simulation ---");
    Serial.println("Target Hash: " + targetHash);
    Serial.println("Starting dictionary attack...\n");
    
    uint32_t startTime = millis();
    bool found = false;
    
    for (int i = 0; i < dictionarySize; i++) {
        String attempt = String(dictionary[i]);
        Serial.print("Trying: '");
        Serial.print(attempt);
        Serial.print("' -> ");
        
        // Use ESP32 hardware SHA acceleration
        String attemptHash = nocShield.hashSHA256(attempt);
        Serial.println(attemptHash);
        
        if (attemptHash.equalsIgnoreCase(targetHash)) {
            Serial.println("\n[SUCCESS] Hash Cracked!");
            Serial.println("Original Password: " + attempt);
            found = true;
            break;
        }
    }
    
    uint32_t endTime = millis();
    
    if (!found) {
        Serial.println("\n[FAILED] Password not found in dictionary.");
    }
    
    Serial.print("Time taken: ");
    Serial.print(endTime - startTime);
    Serial.println(" ms");
}

void loop() {
    delay(1000);
}
