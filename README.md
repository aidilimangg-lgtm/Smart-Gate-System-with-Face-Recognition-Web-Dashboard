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

# 🛠️ Smart Gate System Deployment Guide

This document provides a step-by-step technical guide for configuring your system environment, building the firmware codebases, wiring the electronic hardware layout, and executing initial startup calibration loops.

---

## 📌 Prerequisites & Software Setup

Before proceeding with the hardware assembly, configure your engineering workstation environment:

1. **Install Visual Studio Code (VS Code):** Download and install the latest platform package version.
2. **Install PlatformIO IDE Extension:** Inside VS Code, navigate to the Extensions Marketplace panel, search for `PlatformIO IDE`, and install it.
3. **Download Project Assets:** Ensure the target source repository is checked out locally with the exact folder layout mapping structure specified in the project `README.md`.

---

## 🔌 Step 1: Electrical Hardware Assembly

Wire your components together according to the schematic mapping configuration rules outlined below. 

### ⚡ Critical Power Notice
> **Important:** The integrated camera system processing unit and the continuous gate servo mechanical actuator generate significant peak electrical loads during full operation. Running all modules solely off the micro-USB connection port of your PC can cause line drops, brownouts, and looping boot faults. **Use a high-quality, external 5V regulated power brick to supply the main circuit rails.**

### System Wiring Schematic Matrix

```text
       [ External 5V Power Supply ]
           │              │
           ├──► [5V]      ├──► [5V]
           └──► [GND]     └──► [GND]
                 │              │
        ┌────────┴────────┐    ┌┴────────────────┐
        │    ESP32-CAM    │    │  ESP32 DevKit   │
        │                 │    │                 │
        │  [U0TX] (GPIO1) ├───►│ (GPIO16) [RX2]  │
        │  [U0RX] (GPIO3) ◄───┤ (GPIO17) [TX2]  │
        └─────────────────┘    └─┬────────────┬──┘
                                 │            │
   ┌─────────────────────────────┘            │
   ▼                                          ▼
[ ST7735 1.44" LCD Screen ]             [ Gate Servo Motor ]
  * VCC  ──► 3.3V                         * VCC    ──► External 5V
  * GND  ──► Common GND                   * GND    ──► Common GND
  * CS   ──► GPIO 5                       * Signal ──► GPIO 13
  * RST  ──► GPIO 4
  * DC   ──► GPIO 2
  * SDA  ──► GPIO 23
  * SCK  ──► GPIO 18
