# ESP32 Smart Thermostat & IR Controller for Tronic SMK 7000 C2

This repository contains firmware for an offline ESP32 microcontroller designed to act as an autonomous smart thermostat for the Tronic SMK 7000 C2 air conditioner. It utilizes AHT20 and BMP280 sensors for environmental sensing and controls the AC unit via infrared (IR) signals based on predefined temperature thresholds.

## 🛠 Features
* **Offline Thermostat Logic:** Fully autonomous control without the need for WiFi, MQTT, or external smart home hubs.
* **Environmental Sensing:** Temperature and humidity tracking (AHT20) alongside atmospheric pressure measurement (BMP280).
* **Real-time Visualization:** Displays current metrics and an auto-scaling temperature history graph on a 128x64 OLED screen.
* **Autonomous AC Control:** Built-in thermostat logic utilizing an IR LED to control the AC unit via the NEC protocol.
    * **Power ON Threshold:** >= 25.0 °C
    * **Power OFF Threshold:** <= 23.0 °C

## ⚠️ Known Limitations (Hardware)
The Tronic SMK 7000 C2 air conditioner is an RX-only device and uses a single IR toggle code (`0x39C6`) for its power state. The ESP32 tracks the AC state internally. **Do not use the original AC remote control while this node is active**, as it will cause the ESP32's internal state to desynchronize from the actual physical state of the AC unit.

## ⚙️ Hardware Components
* **Microcontroller:** ESP32 (e.g., WROOM, ESP32-S3, or ESP32-C3)
* **Sensors:** AHT20 (I2C), BMP280 (I2C, Address `0x77`)
* **Display:** 128x64 SSD1306 OLED (I2C, Address `0x3C`)
* **Actuator:** Infrared (IR) LED

## 🔌 Pinout Configuration

| Component | ESP32 Pin | Note |
| :--- | :--- | :--- |
| **VCC** (Sensors & OLED) | `3.3V` | Use 3.3V logic level |
| **GND** (Common) | `GND` | |
| **SDA** (I2C Data) | `GPIO 21` | Default ESP32 I2C |
| **SCL** (I2C Clock) | `GPIO 22` | Default ESP32 I2C |
| **IR LED Data** | `GPIO 15` | Connect via current-limiting resistor / NPN transistor |

## 💻 Software & Installation

The source code is optimized for **PlatformIO**.

**PlatformIO Configuration (`platformio.ini`):**
```ini
[env:esp32dev]
platform = espressif32
board = esp32dev
framework = arduino
lib_deps =
    adafruit/Adafruit AHTX0 @ ^2.0.5
    adafruit/Adafruit BMP280 Library @ ^2.6.8
    adafruit/Adafruit SSD1306 @ ^2.5.10
    adafruit/Adafruit GFX Library @ ^1.11.9
    crankyoldgit/IRremoteESP8266 @ ^2.8.6
