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

// Define the absolute network IP matching your camera node layout deployment
const String camera_ip = "192.168.1.100";

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);
Servo gateServo;
WebServer server(80);

bool gateOpen = false;
unsigned long gateCloseTime = 0;

// Embed the premium index.html asset directly into the flash memory area
const char HTML_DASHBOARD[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Premium Standalone Gate Control</title>
    <script defer src="https://cdn.jsdelivr.net/npm/@vladmandic/face-api/dist/face-api.js"></script>
    <style>
        body { font-family: 'Segoe UI', Tahoma, sans-serif; background: #0e1013; color: #fff; text-align: center; margin: 0; padding: 20px; }
        .container { max-width: 900px; margin: 0 auto; display: flex; flex-wrap: wrap; gap: 20px; justify-content: center; }
        .card { background: #181c24; border-radius: 12px; padding: 20px; box-shadow: 0 4px 15px rgba(0,0,0,0.5); flex: 1; min-width: 350px; border: 1px solid #222834; }
        .video-container { position: relative; width: 400px; height: 300px; margin: 0 auto; border: 2px solid #00ff66; border-radius: 8px; overflow: hidden; background: #000; }
        img#stream { width: 100%; height: 100%; object-fit: cover; }
        .scan-overlay { position: absolute; top: 10%; left: 10%; width: 80%; height: 80%; border: 2px dashed rgba(0, 255, 102, 0.5); animation: pulse 2s infinite; pointer-events: none; }
        @keyframes pulse { 0% { opacity: 0.2; } 50% { opacity: 0.7; } 100% { opacity: 0.2; } }
        .btn { background: #007bff; color: white; border: none; padding: 10px 20px; border-radius: 5px; cursor: pointer; font-size: 15px; margin-top: 10px; transition: 0.3s; font-weight: 600; }
        .btn:hover { background: #0056b3; }
        .user-list { text-align: left; background: #0e1013; padding: 12px; border-radius: 6px; max-height: 150px; overflow-y: auto; border: 1px solid #222834; }
        .status-box { font-size: 22px; font-weight: bold; margin: 15px 0; color: #00ff66; text-transform: uppercase; }
        input[type="text"] { padding: 10px; width: 80%; border-radius: 5px; border: 1px solid #333; background: #0e1013; color: white; margin-bottom: 10px; text-align: center; }
    </style>
</head>
<body>
    <h1>🔮 STANDALONE INTELLIGENT GATE</h1>
    <div class="status-box" id="systemStatus">Loading Security Parameters...</div>
    <div class="container">
        <div class="card">
            <h2>Live Tracking Stream</h2>
            <div class="video-container">
                <img id="stream" src="http://CAMERA_IP_PLACEHOLDER:81/stream" crossorigin="anonymous">
                <div class="scan-overlay"></div>
            </div>
            <button class="btn" style="background:#dc3545;" onclick="triggerManualOpen('OVERRIDE CONTROL')">🚨 Force Open</button>
        </div>
        <div class="card">
            <h2>User Database</h2>
            <input type="text" id="username" placeholder="Type Authorized Name..."><br>
            <label style="color:#aaa; font-size:13px;">Enroll Image File:</label><br>
            <input type="file" id="imageUpload" accept="image/*" class="btn" style="background:#495057;"><br><br>
            <h3>Authorized Entries</h3>
            <div class="user-list" id="userList">No users active.</div>
            <hr style="border:0; border-top:1px solid #222834; margin:20px 0;">
            <button class="btn" style="background:#28a745;" onclick="exportDatabaseToFile()">💾 Export DB</button>
            <input type="file" id="databaseImport" accept=".json" class="btn" style="background:#6c757d;" onchange="importDatabaseFromFile(event)">
        </div>
    </div>
    <script>
        let enrolledUsers = []; let faceMatcher = null; let isLockoutActive = false;
        window.onload = async () => {
            const status = document.getElementById('systemStatus');
            try {
                const modelUrl = 'https://raw.githubusercontent.com/infamousgodhand/face-api.js-models/master/';
                await faceapi.nets.tinyFaceDetector.loadFromUri(modelUrl);
                await faceapi.nets.faceLandmark68Net.loadFromUri(modelUrl);
                await faceapi.nets.faceRecognitionNet.loadFromUri(modelUrl);
                loadProfilesFromLocalStorage();
                status.innerText = "System Biometrics Armed.";
                startFaceDetectionLoop();
            } catch (err) { status.innerText = "Error booting AI assets."; }
        };
        async function startFaceDetectionLoop() {
            const imgElement = document.getElementById('stream');
            setInterval(async () => {
                if (enrolledUsers.length === 0 || !faceMatcher || isLockoutActive) return;
                const detections = await faceapi.detectAllFaces(imgElement, new faceapi.TinyFaceDetectorOptions()).withFaceLandmarks().withFaceDescriptors();
                if (detections.length > 0) {
                    for (const d of detections) {
                        const match = faceMatcher.findBestMatch(d.descriptor);
                        if (match.label !== 'unknown') {
                            document.getElementById('systemStatus').innerText = `Unlocked: ${match.label}`;
                            triggerManualOpen(match.label);
                            isLockoutActive = true;
                            setTimeout(() => { isLockoutActive = false; document.getElementById('systemStatus').innerText = "System Biometrics Armed."; }, 6000);
                            break;
                        }
                    }
                }
            }, 1000);
        }
        function registerNewUser(name, float32Descriptor) {
            enrolledUsers.push({ name: name, descriptor: Array.from(float32Descriptor) });
            rebuildFaceMatcher(); saveToLocalStorageBackup(); updateUserListUI();
        }
        function rebuildFaceMatcher() {
            const labeled = enrolledUsers.map(u => new faceapi.LabeledFaceDescriptors(u.name, [new Float32Array(u.descriptor)]));
            faceMatcher = new faceapi.FaceMatcher(labeled, 0.6);
        }
        function saveToLocalStorageBackup() { localStorage.setItem('gate_auth_vectors', JSON.stringify(enrolledUsers)); }
        function loadProfilesFromLocalStorage() {
            const saved = localStorage.getItem('gate_auth_vectors');
            if (saved) { enrolledUsers = JSON.parse(saved); if(enrolledUsers.length > 0) { rebuildFaceMatcher(); updateUserListUI(); } }
        }
        function exportDatabaseToFile() {
            if(enrolledUsers.length === 0) return;
            const dataStr = "data:text/json;charset=utf-8," + encodeURIComponent(JSON.stringify(enrolledUsers));
            const a = document.createElement('a'); a.setAttribute("href", dataStr); a.setAttribute("download", "gate_profiles.json"); a.click();
        }
        function importDatabaseFromFile(e) {
            const reader = new FileReader();
            reader.onload = function(evt) {
                enrolledUsers = JSON.parse(evt.target.result); rebuildFaceMatcher(); saveToLocalStorageBackup(); updateUserListUI();
            };
            reader.readAsText(e.target.files[0]);
        }
        document.getElementById('imageUpload').addEventListener('change', async (e) => {
            const name = document.getElementById('username').value || "User";
            const img = await faceapi.bufferToImage(e.target.files[0]);
            const det = await faceapi.detectSingleFace(img, new faceapi.TinyFaceDetectorOptions()).withFaceLandmarks().withFaceDescriptor();
            if (det) registerNewUser(name, det.descriptor);
            e.target.value = "";
        });
        function updateUserListUI() {
            document.getElementById('userList').innerHTML = enrolledUsers.map(u => `<div>🟢 Pass Profile: <b>${u.name}</b></div>`).join('');
        }
        async function triggerManualOpen(name) {
            try { await fetch(`/toggle?name=${encodeURIComponent(name)}`); } catch (err) {}
        }
    </script>
</body>
</html>
)rawliteral";

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
    gateServo.write(90); 
    gateOpen = true;
    gateCloseTime = millis() + 5000;
    updateDisplay("GATE OPEN\n\nWELCOME!", user, ST7735_GREEN);
}

void closeGate() {
    gateServo.write(0); 
    gateOpen = false;
    updateDisplay("SYSTEM READY\n\nSCAN FACE", "", ST7735_BLUE);
}

// Serve the internal dashboard code directly down to incoming client requests
void handleRoot() {
    String dynamicHTML = String(HTML_DASHBOARD);
    dynamicHTML.replace("CAMERA_IP_PLACEHOLDER", camera_ip);
    server.send(200, "text/html", dynamicHTML);
}

void handleToggle() {
    server.sendHeader("Access-Control-Allow-Origin", "*");
    String recognizedName = "ADMIN MANUAL";
    if (server.hasArg("name")) {
        recognizedName = server.arg("name");
    }
    openGate(recognizedName);
    server.send(200, "text/plain", "OK");
}

void setup() {
    Serial.begin(115200);
    gateServo.attach(SERVO_PIN);
    gateServo.write(0);

    tft.initR(INITR_144GREENTAB);
    updateDisplay("CONNECTING\nTO NETWORK...", "", ST7735_YELLOW);

    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) { delay(500); }

    // Mount operational routing endpoints maps
    server.on("/", HTTP_GET, handleRoot);
    server.on("/toggle", HTTP_GET, handleToggle);
    server.begin();
    
    closeGate();
}

void loop() {
    server.handleClient();
    if (gateOpen && millis() > gateCloseTime) {
        closeGate();
    }
}
