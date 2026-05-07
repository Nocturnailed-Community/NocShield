#include <Arduino.h>
#include "NocShield.h"

NocShield nocShield;
uint32_t packetCount = 0;

// Callback function to process intercepted packets
void packetMonitorCallback(void* buf, wifi_promiscuous_pkt_type_t type) {
    wifi_promiscuous_pkt_t* pkt = (wifi_promiscuous_pkt_t*)buf;
    
    // Only process management frames (type 0) for this example
    if (type == WIFI_PKT_MGMT) {
        packetCount++;
        
        // Print an update every 100 packets
        if (packetCount % 100 == 0) {
            Serial.printf("Captured %u management packets so far. Last RSSI: %d\n", packetCount, pkt->rx_ctrl.rssi);
        }
    }
}

void setup() {
    Serial.begin(115200);
    delay(2000);
    Serial.println("\n--- NocShield Packet Monitor Simulation ---");
    
    // Initialize NocShield
    nocShield.begin();
    
    // Start promiscuous mode and register our callback
    nocShield.startPacketMonitor(packetMonitorCallback);
    
    Serial.println("Packet monitor started. Listening for management frames...");
}

void loop() {
    // The packet monitoring is handled asynchronously by the callback.
    // We can do other things here or just wait.
    delay(1000);
}
