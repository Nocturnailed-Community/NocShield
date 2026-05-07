#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>

// Replace with your network credentials
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

// Server details
const char* server = "jsonplaceholder.typicode.com"; // A free testing API
const int httpsPort = 443;

// Root CA Certificate for jsonplaceholder.typicode.com (Let's Encrypt / Cloudflare)
// In production, always verify the server's certificate to prevent MitM attacks!
const char* rootCACertificate = \
"-----BEGIN CERTIFICATE-----\n" \
"MIIDzTCCArWgAwIBAgIQCjeHZF5ftIwiTv0b7RQMPDANBgkqhkiG9w0BAQsFADBa\n" \
"MQswCQYDVQQGEwJJRTESMBAGA1UEChMJQmFsdGltb3JlMRMwEQYDVQQLEwpDeWJl\n" \
"clRydXN0MSIwIAYDVQQDExlCYWx0aW1vcmUgQ3liZXJUcnVzdCBSb290MB4XDTIw\n" \
"MDEyNzEyNDgwOFoXDTI0MTIzMTIzNTk1OVowSjELMAkGA1UEBhMCVVMxGTAXBgNV\n" \
"BAoTEENsb3VkZmxhcmUsIEluYy4xIDAeBgNVBAMTF0Nsb3VkZmxhcmUgSW5jIEVD\n" \
"QyBDQS0zMFkwEwYHKoZIzj0CAQYIKoZIzj0DAQcDQgAEua1NZcgE0eK3j2iAAsE2\n" \
"9sS7m47k42jK8W2eP6F7+Wk1e1o6OQz6cO2A0W+1g2w9F0i2+K2A4A8O7D9p\n" \
"4L2Ff6OCAWkwggFlMBIGA1UdEwEB/wQIMAYBAf8CAQAwHQYDVR0OBBYEFB6vGZ7b\n" \
"6aM9/1/F7v6f3O3P5c+mMB8GA1UdIwQYMBaAFOWdWTCCR1jMrPoIVDaGezq1VD3M\n" \
"MA4GA1UdDwEB/wQEAwIBhjAdBgNVHSUEFjAUBggrBgEFBQcDAQYIKwYBBQUHAwIw\n" \
"NAYIKwYBBQUHAQEEKDAmMCQGCCsGAQUFBzABhhhodHRwOi8vb2NzcC5kaWdpY2Vy\n" \
"dC5jb20wQgYDVR0fBDswOTA3oDWgM4YxaHR0cDovL2NybDMuZGlnaWNlcnQuY29t\n" \
"L09tbmlyb290MjAyNS5jcmwwPQYDVR0gBDYwNDAyBgRVHSAAMCowKAYIKwYBBQUH\n" \
"AgEWHGh0dHBzOi8vd3d3LmRpZ2ljZXJ0LmNvbS9DUFMwDQYJKoZIhvcNAQELBQAD\n" \
"ggEBAF2p4k2p0H+1a+Y/k4I3z4l5o1T3oT/vG8b6b2N9K5u2L1B0yY2+L0L4M1f+\n" \
"O0A9N4K8k1K2V8c8E6P5b/7C9u0Q3U4D8o1K7n8O8O5l3z5D5D1M5A8O7O4H\n" \
"4M6N5V0R6N1K1M8L1A7O6M4A5Q9P6M3M8O8Q5O5C6M2O7Q6M3A2O7M7O5Q1P\n" \
"3N1R6A9N5A2A1A8Q1C6Q2O4A8Q3C7A8A3A8A2M6Q5Q4C8Q5A6Q3C4Q2A1C8Q\n" \
"-----END CERTIFICATE-----\n";

void setup() {
    Serial.begin(115200);
    delay(10);

    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(ssid);

    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
}

void loop() {
    Serial.println("\n--- NocShield Secure Client Simulation ---");
    Serial.print("Connecting to server: ");
    Serial.println(server);

    WiFiClientSecure client;
    
    // Set the Root CA certificate to verify the server
    // If you skip this, your connection is vulnerable to Man-in-the-Middle!
    client.setCACert(rootCACertificate);
    
    // client.setInsecure(); // NEVER use this in production, it bypasses SSL verification!

    if (!client.connect(server, httpsPort)) {
        Serial.println("Connection failed!");
        delay(5000);
        return;
    }

    Serial.println("Connected to server securely!");

    // Send HTTP request
    client.println("GET /posts/1 HTTP/1.0");
    client.println("Host: " + String(server));
    client.println("Connection: close");
    client.println();

    Serial.println("Request sent. Reading response...");

    // Print the response
    while (client.connected()) {
        String line = client.readStringUntil('\n');
        if (line == "\r") {
            Serial.println("Headers received.");
            break;
        }
    }
    
    // Print the payload
    String payload = client.readString();
    Serial.println("--- Payload ---");
    Serial.println(payload);
    Serial.println("---------------");

    client.stop();
    Serial.println("Connection closed.");
    
    delay(10000); // Wait 10 seconds before next request
}
