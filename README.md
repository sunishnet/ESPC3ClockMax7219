# ESP32-C3 Super Mini MAX7219 4-in-1 NTP Matrix Clock

A modern, fast, and feature-rich NTP Digital Matrix Clock built for the **ESP32-C3 Super Mini** microcontroller and **MAX7219 4-in-1 Dot Matrix Display Module** (32x8 LED matrix).

![ESP32-C3 MAX7219 Clock](https://img.shields.io/badge/ESP32--C3-Super%20Mini-blue) ![MAX7219](https://img.shields.io/badge/Display-MAX7219%204in1-red) ![WiFi](https://img.shields.io/badge/WiFi-SSID%204D-green) ![mDNS](https://img.shields.io/badge/mDNS-clock.local-orange)

---

## ✨ Features

- **Direct Hardcoded WiFi**: Configured to connect directly to WiFi SSID `4D`.
- **NTP Time Synchronization**: Automatic time syncing using ESP32 native POSIX timezone handlers with full Daylight Saving Time (DST) support.
- **mDNS Support**: Access the device control dashboard at **`http://clock.local`** directly from any phone or PC on your local network.
- **Embedded Web Control Panel**:
  - Live clock display & connection status.
  - Real-time brightness control slider (0 to 15).
  - 12-Hour / 24-Hour time format toggle.
  - Periodic date scrolling interval.
  - Dropdown menu with pre-configured POSIX timezones (IST, EST, CST, PST, GMT, CET, JST, AEST, UTC) or custom string.
  - Instant NTP sync button and System Restart button.
- **Persistent Memory**: All settings stored safely in ESP32 `Preferences` non-volatile flash storage.

---

## 🔌 Hardware Wiring & Schematic

Connect the **ESP32-C3 Super Mini** board to the **MAX7219 4-in-1 Dot Matrix Display**:

| MAX7219 Pin | ESP32-C3 Super Mini Pin | Notes |
| :--- | :--- | :--- |
| **VCC** | **5V / 5V_IN** | 5V power supply |
| **GND** | **GND** | Ground connection |
| **DIN** | **GPIO 7** | Data (MOSI) |
| **CLK** | **GPIO 6** | Clock (SCK) |
| **CS / LOAD** | **GPIO 5** | Chip Select (Load) |

---

## 🛠️ Build & Upload Guide

### Method A: PlatformIO (Recommended - Fast Upload)

1. Open this repository folder in **VS Code** with the **PlatformIO** extension installed.
2. Build and upload using terminal:
   ```bash
   pio run -t upload
   ```
3. Monitor serial output:
   ```bash
   pio device monitor
   ```

---

### Method B: Arduino IDE

1. **Required Libraries** (`Tools > Manage Libraries`):
   - **MD_Parola**
   - **MD_MAX72XX**
   - **ArduinoJson**

2. **Board Settings**:
   - Board: `ESP32C3 Dev Module`
   - USB CDC On Boot: `Enabled`
   - Upload Speed: `921600`

3. Open `ESPC3ClockMax7219.ino` and click **Upload**.

---

## 🌐 Web Dashboard Access

Once connected to your WiFi network:
- Open your browser and go to: **`http://clock.local`**
