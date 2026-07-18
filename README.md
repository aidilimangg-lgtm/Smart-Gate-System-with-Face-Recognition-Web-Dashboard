# 🚪 Smart Gate System with Face Recognition & Web Dashboard

An automated smart gate access control system built using a dual-MCU setup (**ESP32-CAM** + **ESP32 DevKit**). The project combines on-device local face recognition using vector matrix models with an expandable storage architecture via MicroSD card, an SPI status display, a servo driver, and a centralized operational web dashboard.

---

## 🚀 Features
*   **Dual-Board Processing Distributed Architecture:** Splitting intensive video/biometric computation from the hardware peripheral control loops ensures high uptime and avoids system lockups.
*   **Persistent Vector Storage:** Leverages the internal MicroSD slot using an optimized 1-bit bus configuration to save mathematical face models, bypassing default RAM limits.
*   **Real-time Local Status Monitoring:** Features an ST7735 1.44-inch color LCD display providing live visual metrics of the gate's mechanical and networking states.
*   **Central Web Dashboard Control:** Unified control surface serving an administrative console featuring an inline low-latency video stream pipeline and digital "Force Open" capability.

---

## 🔌 Hardware Circuit Architecture

The boards communicate asynchronously via UART over their secondary serial interface. A common ground reference must be shared across all components.

### 1. Pin Configuration Mappings

| ESP32-CAM Pin | Connection / Target | Description |
| :--- | :--- | :--- |
| **VCC** | 5V External Supply | Main power rail input |
| **GND** | System Common GND | Shared electrical ground |
| **TX (GPIO 1)** | ESP32 DevKit **RX2 (GPIO 16)** | UART Communication Uplink |
| **RX (GPIO 3)** | ESP32 DevKit **TX2 (GPIO 17)** | UART Communication Downlink |

| ESP32 DevKit Pin | Peripheral Component | Peripheral Pin |
| :--- | :--- | :--- |
| **GPIO 13** | Servo Motor | Signal (PWM) |
| **GPIO 5** | ST7735 1.44" LCD Display | CS (Chip Select) |
| **GPIO 4** | ST7735 1.44" LCD Display | RESET |
| **GPIO 2** | ST7735 1.44" LCD Display | A0 / DC (Data/Command) |
| **GPIO 23** | ST7735 1.44" LCD Display | SDA / MOSI |
| **GPIO 18** | ST7735 1.44" LCD Display | SCK / CLK |
| **3.3V** | ST7735 1.44" LCD Display | VCC |
| **GND** | ST7735 1.44" LCD Display | GND |

> ⚠️ **Important:** Using the integrated MicroSD slot on the ESP32-CAM shares physical data traces with the onboard flash LED (`GPIO 4`). Flashing operations are constrained to 1-bit line access mode inside the initialization blocks to prevent image frame allocation corruption.

---

## 🛠️ Software Installation & Compilation

This application is built natively under the **PlatformIO** ecosystem inside VS Code. 

### 1. Project Directory Tree Structure
Ensure your local project directory is configured as follows before deploying code:
```text
smart-gate-system/
├── lib/
├── src/
│   ├── main_devkit.cpp    # Upload to ESP32 DevKit
│   └── main_cam.cpp       # Upload to ESP32-CAM
├── platformio.ini         # Target configuration profile
└── LICENSE
