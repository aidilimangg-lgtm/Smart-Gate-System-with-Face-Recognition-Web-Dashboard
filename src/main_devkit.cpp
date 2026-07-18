#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <Servo.h>

// WiFi Settings
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

// Pin Mappings for ST7735 1.44" Display
#define TFT_CS    5
#define TFT_RST   4
#define TFT_DC    2
#define TFT_MOSI  23 // SDA pin on screen
#define TFT_SCLK  18 // CLK pin on screen

// Pin Mapping for Servo Motor
#define SERVO_PIN 13

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);
Servo gateServo;
WebServer server(80);

bool gateOpen = false;
unsigned long gateCloseTime = 0;

// Updates screen text and colors cleanly without screen burn/flicker
void updateDisplay(String status, uint16_t color) {
    tft.fillScreen(ST7735_BLACK);
    tft.setCursor(0, 10);
    tft.setTextColor(ST7735_WHITE);
    tft.setTextSize(1);
    tft.println("--- AI SMART GATE ---");
    
    tft.setCursor(0, 45);
    tft.setTextSize(2);
    tft.setTextColor(color);
    tft.println(status);
}

void openGate() {
    gateServo.write(90); // Adjust mechanical opening angle if needed
    gateOpen = true;
    gateCloseTime = millis() + 5000; // Automatic close countdown sequence set to 5s
    updateDisplay("FACE\nRECOGNIZED\n\nGATE OPENING", ST7735_GREEN);
}

void closeGate() {
    gateServo.write(0); // Lock positions back down to zero
    gateOpen = false;
    updateDisplay("SYSTEM READY\n\nSCAN FACE", ST7735_BLUE);
}

void handleToggle() {
    // Inject CORS headers so your browser index.html dashboard can trigger the API safely
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.sendHeader("Access-Control-Allow-Headers", "*");
    openGate();
    server.send(200, "text/plain", "GATE_OPENING_TRIGGERED");
}

void setup() {
    Serial.begin(115200);
    
    // Setup Gate Actuator
    gateServo.attach(SERVO_PIN);
    gateServo.write(0);

    // Setup ST7735 Screen
    tft.initR(INITR_144GREENTAB);
    updateDisplay("CONNECTING\nTO NETWORK...", ST7735_YELLOW);

    // Setup Local WiFi Configuration Mappings
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) { 
        delay(500); 
        Serial.print("."); 
    }
    
    Serial.println("\nWiFi Connected!");
    
    server.on("/toggle", HTTP_GET, handleToggle);
    server.begin();

    // Display System Ready Initial Screen State 
    closeGate();
}

void loop() {
    server.handleClient();

    // Keep loop tracking safe to auto-close gate via non-blocking timers
    if (gateOpen && millis() > gateCloseTime) {
        closeGate();
    }
}
