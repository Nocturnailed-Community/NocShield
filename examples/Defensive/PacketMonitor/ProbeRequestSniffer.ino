/*
 * NocShield - Probe Request Sniffer (Defensive)
 * ------------------------------------------------
 * Passively listens for 802.11 Probe Request management frames and logs
 * the sender's MAC address and the SSID it is looking for (if any).
 *
 * This is a MONITORING tool for auditing your own environment, e.g.:
 *   - Spotting unknown/rogue devices probing near your equipment.
 *   - Verifying your own devices aren't leaking previously-used SSIDs.
 *   - Basic Wi-Fi site-survey / device-count diagnostics.
 *
 * It does NOT transmit anything and cannot disconnect or attack any
 * device - it only reads frames that are already broadcast in the air.
 *
 * PRIVACY NOTE: Probe requests are broadcast in the clear by nearby
 * devices, but the MAC addresses and SSID history they reveal can still
 * identify people/devices. Only run this on premises you control or are
 * authorized to monitor, and don't use the collected data to track or
 * identify individuals.
 */

#include <Arduino.h>
#include "NocShield.h"

NocShield nocShield;

// How many distinct probing devices to remember (simple ring buffer)
#define MAX_TRACKED_DEVICES 32

struct ProbeEntry {
    uint8_t mac[6];
    uint32_t lastSeenMs;
    uint32_t count;
    bool used;
};

ProbeEntry seenDevices[MAX_TRACKED_DEVICES];

// 802.11 management frame subtype for Probe Request
static const uint8_t SUBTYPE_PROBE_REQUEST = 0x40;

// Minimal 802.11 management frame header layout we need:
//  [0]     frame control (subtype in high nibble of first byte's low bits)
//  [4..9]  destination address
//  [10..15] source address (this is the probing device's MAC)
//  [16..21] BSSID
//  [24..]  frame body (for probe request: SSID element starts here)
struct __attribute__((packed)) wifi_ieee80211_mac_hdr_t {
    uint16_t frame_ctrl;
    uint16_t duration_id;
    uint8_t addr1[6];
    uint8_t addr2[6];
    uint8_t addr3[6];
    uint16_t seq_ctrl;
};

int findOrAddDevice(const uint8_t* mac) {
    // Look for an existing entry
    for (int i = 0; i < MAX_TRACKED_DEVICES; i++) {
        if (seenDevices[i].used && memcmp(seenDevices[i].mac, mac, 6) == 0) {
            return i;
        }
    }
    // Add into the first free slot, or overwrite the oldest one
    int oldestIdx = 0;
    uint32_t oldestTime = 0xFFFFFFFF;
    for (int i = 0; i < MAX_TRACKED_DEVICES; i++) {
        if (!seenDevices[i].used) {
            return i;
        }
        if (seenDevices[i].lastSeenMs < oldestTime) {
            oldestTime = seenDevices[i].lastSeenMs;
            oldestIdx = i;
        }
    }
    return oldestIdx;
}

String macToString(const uint8_t* mac) {
    char buf[18];
    snprintf(buf, sizeof(buf), "%02X:%02X:%02X:%02X:%02X:%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    return String(buf);
}

void probeRequestCallback(void* buf, wifi_promiscuous_pkt_type_t type) {
    if (type != WIFI_PKT_MGMT) return;

    wifi_promiscuous_pkt_t* pkt = (wifi_promiscuous_pkt_t*)buf;
    const uint8_t* payload = pkt->payload;

    // Frame control's subtype lives in bits 4-7 of the first byte.
    uint8_t subtype = payload[0] & 0xF0;
    if (subtype != SUBTYPE_PROBE_REQUEST) return;

    const wifi_ieee80211_mac_hdr_t* hdr = (const wifi_ieee80211_mac_hdr_t*)payload;
    const uint8_t* srcMac = hdr->addr2;

    // SSID element (tag 0) starts right after the 24-byte MAC header.
    const uint8_t* ssidTag = payload + sizeof(wifi_ieee80211_mac_hdr_t);
    uint8_t ssidLen = 0;
    String ssid = "<broadcast/hidden>";

    if (pkt->rx_ctrl.sig_len > sizeof(wifi_ieee80211_mac_hdr_t) + 2 && ssidTag[0] == 0x00) {
        ssidLen = ssidTag[1];
        if (ssidLen > 0 && ssidLen <= 32) {
            char ssidBuf[33];
            memcpy(ssidBuf, &ssidTag[2], ssidLen);
            ssidBuf[ssidLen] = '\0';
            ssid = String(ssidBuf);
        }
    }

    int idx = findOrAddDevice(srcMac);
    bool isNew = !seenDevices[idx].used;
    memcpy(seenDevices[idx].mac, srcMac, 6);
    seenDevices[idx].lastSeenMs = millis();
    seenDevices[idx].count = isNew ? 1 : seenDevices[idx].count + 1;
    seenDevices[idx].used = true;

    if (isNew) {
        Serial.printf("[NEW DEVICE] %s probing for SSID: \"%s\" (RSSI %d)\n",
                      macToString(srcMac).c_str(), ssid.c_str(), pkt->rx_ctrl.rssi);
    }
}

void setup() {
    Serial.begin(115200);
    delay(2000);

    memset(seenDevices, 0, sizeof(seenDevices));

    Serial.println("\n--- NocShield Probe Request Sniffer (Defensive) ---");
    Serial.println("Passively listening for nearby devices' Wi-Fi probe requests...");

    nocShield.begin();
    nocShield.startPacketMonitor(probeRequestCallback);
}

void loop() {
    static uint32_t lastReport = 0;
    if (millis() - lastReport > 10000) {
        lastReport = millis();
        int activeCount = 0;
        for (int i = 0; i < MAX_TRACKED_DEVICES; i++) {
            if (seenDevices[i].used) activeCount++;
        }
        Serial.printf("[SUMMARY] %d distinct device(s) tracked in the last window.\n", activeCount);
    }
    delay(100);
}
