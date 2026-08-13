# ESP32 Climate Monitor & IR Controller for Tronic SMK 7000 C2

This repository contains firmware for an ESP32 microcontroller designed to monitor environmental telemetry (temperature, humidity, atmospheric pressure) using AHT20 and BMP280 sensors. It features an SSD1306 OLED display for real-time data visualization and an IR transmitter for autonomous control of the Tronic SMK 7000 C2 air conditioner based on predefined temperature thresholds.

## 🛠 Features
*   **Environmental Telemetry:** High-precision temperature and humidity tracking (AHT20) alongside atmospheric pressure measurement (BMP280).
*   **Real-time Visualization:** Displays current metrics and an auto-scaling temperature history graph on a 128x64 OLED screen.
*   **Autonomous AC Control:** Built-in thermostat logic utilizing an IR LED to control the AC unit via the NEC protocol.
    *   **Power ON Threshold:** ≥ 25.0 °C
    *   **Power OFF Threshold:** ≤ 23.0 °C

## ⚙️ Hardware Components
*   **Microcontroller:** ESP32 (e.g., WROOM, ESP32-S3, or ESP32-C3)
*   **Sensors:** AHT20 (I2C), BMP280 (I2C, Address `0x77`)
*   **Display:** 128x64 SSD1306 OLED (I2C, Address `0x3C`)
*   **Actuator:** Infrared (IR) LED

## 🔌 Pinout Configuration

The sensors and the OLED display communicate via the I2C bus. The IR transmitter requires a standard digital output pin.

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
```

**Setup Instructions:**
1.  Clone this repository: `git clone https://github.com/stofacz271/ESP32_BMP280_and_AHT20_sensor.git`
2.  Open the project directory in Visual Studio Code with the PlatformIO extension installed.
3.  PlatformIO will automatically fetch and install all required libraries defined in the `platformio.ini` file.
4.  Compile and upload the firmware to the ESP32.
5.  The system will boot, initialize the I2C bus, search for the sensors, and begin autonomous climate control.

## 📡 IR Command Structure
The firmware uses the 32-bit NEC protocol. The definitions are structured as `usercode << 16 | datacode`.

*   **Address:** `0x01FE`
*   **Power Command:** `0x39C6` *(Used for state toggling)*
