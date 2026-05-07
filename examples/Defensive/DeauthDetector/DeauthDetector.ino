#include <Arduino.h>
#include "NocShield.h"

NocShield nocShield;

// Alarm LED pin (built-in LED on most ESP32 dev boards is GPIO 2)
const int ALARM_PIN = 2;

// Variables for detecting deauth storms
volatile uint32_t deauthCount = 0;
uint32_t lastCheckTime = 0;
const uint32_t CHECK_INTERVAL_MS = 1000; // Check every second
const uint32_t DEAUTH_THRESHOLD = 50; // Threshold per second to trigger alarm

// Callback function to process intercepted packets
void packetMonitorCallback(void* buf, wifi_promiscuous_pkt_type_t type) {
    if (type == WIFI_PKT_MGMT) {
        wifi_promiscuous_pkt_t* pkt = (wifi_promiscuous_pkt_t*)buf;
        uint8_t* frame = pkt->payload;
        
        // Frame Control field is the first 2 bytes.
        // For Deauth, subtype is 1100 (0x0C) and type is 00 (Management) -> 0xC0
        // For Disassociation, subtype is 1010 (0x0A) -> 0xA0
        if (frame[0] == 0xC0 || frame[0] == 0xA0) {
            deauthCount++;
        }
    }
}

void setup() {
    Serial.begin(115200);
    pinMode(ALARM_PIN, OUTPUT);
    digitalWrite(ALARM_PIN, LOW);
    
    delay(2000);
    Serial.println("\n--- NocShield Deauth Detector Simulation ---");
    
    // Initialize NocShield
    nocShield.begin();
    
    // Start promiscuous mode and register our callback
    nocShield.startPacketMonitor(packetMonitorCallback);
    
    Serial.println("Monitoring for Deauth attacks...");
}

void loop() {
    uint32_t currentTime = millis();
    
    if (currentTime - lastCheckTime >= CHECK_INTERVAL_MS) {
        // Temporarily disable interrupts or store the value to avoid race conditions
        uint32_t count = deauthCount; 
        deauthCount = 0; // Reset counter
        lastCheckTime = currentTime;
        
        if (count > 0) {
            Serial.printf("Deauth/Disassoc packets detected: %u/s\n", count);
        }
        
        if (count > DEAUTH_THRESHOLD) {
            Serial.println("[WARNING] DEAUTH ATTACK DETECTED! ALARM TRIGGERED!");
            digitalWrite(ALARM_PIN, HIGH); // Turn on LED
            delay(500); // Keep on for half a second to flash
            digitalWrite(ALARM_PIN, LOW);
        }
    }
}
