/*
 * NocShield - Deauth Detector (Defensive) - v2
 * ------------------------------------------------
 * Enhancements over the original example:
 *   1. Tracks deauth/disassoc counts PER SOURCE MAC, so you can identify
 *      which AP/device is actually sending the flood, instead of only a
 *      global counter.
 *   2. Optional whitelist so your own AP's normal disassoc traffic
 *      (e.g. clients roaming/sleeping) doesn't trigger false alarms.
 *   3. Same LED alarm behavior as the original, kept for compatibility.
 *
 * This is purely a passive listener (promiscuous mode) - it never
 * transmits anything and cannot be used to attack a network.
 */

#include <Arduino.h>
#include "NocShield.h"

NocShield nocShield;

// Alarm LED pin (built-in LED on most ESP32 dev boards is GPIO 2)
const int ALARM_PIN = 2;

const uint32_t CHECK_INTERVAL_MS = 1000;   // Check every second
const uint32_t DEAUTH_THRESHOLD = 50;      // Global threshold per second to trigger alarm
const uint32_t PER_SOURCE_THRESHOLD = 20;  // Per-attacker threshold per second

// Optional: MACs to ignore (e.g. your own router doing normal housekeeping).
// Add entries as {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF}.
uint8_t whitelist[][6] = {
    // { 0x00, 0x11, 0x22, 0x33, 0x44, 0x55 },
};
const size_t whitelistCount = sizeof(whitelist) / sizeof(whitelist[0]);

#define MAX_TRACKED_SOURCES 16
struct SourceCount {
    uint8_t mac[6];
    uint32_t count;
    bool used;
};

volatile SourceCount sources[MAX_TRACKED_SOURCES];
volatile uint32_t totalCount = 0;
uint32_t lastCheckTime = 0;

bool isWhitelisted(const uint8_t* mac) {
    for (size_t i = 0; i < whitelistCount; i++) {
        if (memcmp(whitelist[i], mac, 6) == 0) return true;
    }
    return false;
}

String macToString(const uint8_t* mac) {
    char buf[18];
    snprintf(buf, sizeof(buf), "%02X:%02X:%02X:%02X:%02X:%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    return String(buf);
}

void packetMonitorCallback(void* buf, wifi_promiscuous_pkt_type_t type) {
    if (type != WIFI_PKT_MGMT) return;

    wifi_promiscuous_pkt_t* pkt = (wifi_promiscuous_pkt_t*)buf;
    uint8_t* frame = pkt->payload;

    // Deauth (subtype 0x0C -> 0xC0) or Disassociation (subtype 0x0A -> 0xA0)
    if (frame[0] != 0xC0 && frame[0] != 0xA0) return;

    // Source address (addr2) is bytes 10-15 of the 802.11 MAC header.
    const uint8_t* srcMac = &frame[10];

    if (isWhitelisted(srcMac)) return;

    totalCount++;

    for (int i = 0; i < MAX_TRACKED_SOURCES; i++) {
        if (sources[i].used && memcmp((void*)sources[i].mac, srcMac, 6) == 0) {
            sources[i].count++;
            return;
        }
    }
    // Not found: place into an empty slot if available
    for (int i = 0; i < MAX_TRACKED_SOURCES; i++) {
        if (!sources[i].used) {
            memcpy((void*)sources[i].mac, srcMac, 6);
            sources[i].count = 1;
            sources[i].used = true;
            return;
        }
    }
    // Table full: drop silently (rare edge case with many simultaneous sources)
}

void setup() {
    Serial.begin(115200);
    pinMode(ALARM_PIN, OUTPUT);
    digitalWrite(ALARM_PIN, LOW);

    delay(2000);
    Serial.println("\n--- NocShield Deauth Detector v2 (per-source tracking) ---");

    nocShield.begin();
    nocShield.startPacketMonitor(packetMonitorCallback);

    Serial.println("Monitoring for Deauth/Disassoc storms...");
}

void loop() {
    uint32_t currentTime = millis();

    if (currentTime - lastCheckTime >= CHECK_INTERVAL_MS) {
        lastCheckTime = currentTime;

        uint32_t count = totalCount;
        totalCount = 0;

        if (count > 0) {
            Serial.printf("Deauth/Disassoc packets detected: %u/s\n", count);
        }

        bool alarm = count > DEAUTH_THRESHOLD;

        for (int i = 0; i < MAX_TRACKED_SOURCES; i++) {
            if (sources[i].used && sources[i].count > 0) {
                if (sources[i].count > PER_SOURCE_THRESHOLD) {
                    Serial.printf("[SUSPECT] %s sent %u deauth/disassoc frames in the last second\n",
                                  macToString((uint8_t*)sources[i].mac).c_str(), sources[i].count);
                    alarm = true;
                }
                sources[i].count = 0; // reset window, keep the slot for next second
            }
        }

        if (alarm) {
            Serial.println("[WARNING] DEAUTH ATTACK DETECTED! ALARM TRIGGERED!");
            digitalWrite(ALARM_PIN, HIGH);
            delay(500);
            digitalWrite(ALARM_PIN, LOW);
        }
    }
}
