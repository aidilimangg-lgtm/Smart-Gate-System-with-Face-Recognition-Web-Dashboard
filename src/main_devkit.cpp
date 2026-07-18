#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <Servo.h>

const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

#define TFT_CS    5
#define TFT_RST   4
#define TFT_DC    2
#define SERVO_PIN 13

String camera_ip = "192.168.1.100"; // Replace with your target ESP32-CAM IP address

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);
Servo gateServo;
WebServer server(80);

bool gateOpen = false;
unsigned long gateCloseTime = 0;

void updateDisplay(String status, uint16_t color) {
    tft.fillScreen(ST7735_BLACK);
    tft.setCursor(0, 10);
    tft.setTextColor(ST7735_WHITE);
    tft.setTextSize(1);
    tft.println("--- GATE SYSTEM ---");
    tft.setCursor(0, 40);
    tft.setTextSize(2);
    tft.setTextColor(color);
    tft.println(status);
}

void openGate() {
    gateServo.write(90); 
    gateOpen = true;
    gateCloseTime = millis() + 5000; 
    updateDisplay("GATE OPEN\nWELCOME!", ST7735_GREEN);
}

void closeGate() {
    gateServo.write(0);
    gateOpen = false;
    updateDisplay("GATE CLOSED\nSCAN FACE", ST7735_RED);
}

void handleRoot() {
    String html = "<html><head><title>Gate Dashboard</title>";
    html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
    html += "<style>body{font-family:Arial; text-align:center; background:#f4f4f4;} .btn{padding:15px 25px; font-size:18px; color:white; background:#007BFF; border:none; border-radius:5px; cursor:pointer;}</style></head>";
    html += "<body><h1>Smart Gate Dashboard</h1>";
    html += "<div><h3>Live View</h3><img src='http://" + camera_ip + ":81/stream' style='width:100%; max-width:400px; border:3px solid #333;' /></div>";
    html += "<br><br><button class='btn' onclick=\"fetch('/toggle')\">Force Open Gate</button>";
    html += "</body></html>";
    server.send(200, "text/html", html);
}

void handleToggle() {
    openGate();
    server.send(200, "text/plain", "Triggered");
}

void setup() {
    Serial.begin(115200);
    Serial2.begin(115200, SERIAL_8N1, 16, 17); 

    gateServo.attach(SERVO_PIN);
    gateServo.write(0);

    tft.initR(INITR_144GREENTAB);
    updateDisplay("CONNECTING\nTO WIFI...", ST7735_YELLOW);

    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) { delay(500); }

    server.on("/", handleRoot);
    server.on("/toggle", handleToggle);
    server.begin();

    updateDisplay("SYSTEM READY\nSCAN FACE", ST7735_BLUE);
}

void loop() {
    server.handleClient();

    if (Serial2.available()) {
        String msg = Serial2.readStringUntil('\n');
        msg.trim();
        if (msg == "MATCH_FOUND") {
            openGate();
        }
    }

    if (gateOpen && millis() > gateCloseTime) {
        closeGate();
    }
}
