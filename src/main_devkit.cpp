#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <Servo.h>

// WiFi Configuration Metrics
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

// Pin Mappings for ST7735 1.44" Display
#define TFT_CS    5
#define TFT_RST   4
#define TFT_DC    2
#define TFT_MOSI  23 // SDA pin mapping
#define TFT_SCLK  18 // CLK pin mapping

// Pin Mapping for Actuator Servo Motor
#define SERVO_PIN 13

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);
Servo gateServo;
WebServer server(80);

bool gateOpen = false;
unsigned long gateCloseTime = 0;

void updateDisplay(String status, String name, uint16_t color) {
    tft.fillScreen(ST7735_BLACK);
    tft.setCursor(0, 10);
    tft.setTextColor(ST7735_WHITE);
    tft.setTextSize(1);
    tft.println("--- AI SMART GATE ---");
    
    tft.setCursor(0, 40);
    tft.setTextSize(2);
    tft.setTextColor(color);
    tft.println(status);

    if (name.length() > 0) {
        tft.setCursor(0, 105);
        tft.setTextSize(1);
        tft.setTextColor(ST7735_WHITE);
        tft.print("User: ");
        tft.setTextColor(color);
        tft.println(name);
    }
}

void openGate(String user) {
    gateServo.write(90); // Mechanical gate opening sweep angle configuration
    gateOpen = true;
    gateCloseTime = millis() + 5000; // Hard expiration timer sequence set to 5 seconds
    updateDisplay("GATE OPEN\n\nWELCOME!", user, ST7735_GREEN);
    Serial.println("[SYSTEM] Gate State Matrix: OPENED BY " + user);
}

void closeGate() {
    gateServo.write(0); // Restore mechanical baseline tracking alignment
    gateOpen = false;
    updateDisplay("SYSTEM READY\n\nSCAN FACE", "", ST7735_BLUE);
    Serial.println("[SYSTEM] Gate State Matrix: SECURED");
}

void handleToggle() {
    // Inject Cross-Origin Resource Sharing (CORS) interface definitions
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.sendHeader("Access-Control-Allow-Headers", "*");
    
    // Parse dynamic named identity query parameters mapping arrays
    String recognizedName = "ADMIN CONTROL";
    if (server.hasArg("name")) {
        recognizedName = server.arg("name");
    }
    
    openGate(recognizedName);
    server.send(200, "text/plain", "GATE_OPEN_SUCCESS");
}

void setup() {
    Serial.begin(115200);
    gateServo.attach(SERVO_PIN);
    gateServo.write(0);

    tft.initR(INITR_144GREENTAB);
    updateDisplay("CONNECTING\nTO NETWORK...", "", ST7735_YELLOW);

    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) { 
        delay(500); 
        Serial.print(".");
    }
    
    Serial.println("\n[NET] Control Node Stack Online IP: ");
    Serial.println(WiFi.localIP());

    server.on("/toggle", HTTP_GET, handleToggle);
    server.begin();
    
    closeGate();
}

void loop() {
    server.handleClient();
    
    // Safety auto-recovery sequence logic checkpoints
    if (gateOpen && millis() > gateCloseTime) {
        closeGate();
    }
}
