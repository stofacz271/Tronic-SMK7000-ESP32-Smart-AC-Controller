#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_AHTX0.h>
#include <Adafruit_BMP280.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
#define OLED_ADDR     0x3C

Adafruit_AHTX0 aht;
Adafruit_BMP280 bmp;
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Stav senzorů
bool ahtOK = false;
bool bmpOK = false;

// Interval čtení
const unsigned long READ_INTERVAL = 2000;
unsigned long lastRead = 0;

// Naměřené hodnoty
float tempAHT = 0, humidity = 0;
float tempBMP = 0, pressure = 0, altitude = 0;

// Referenční tlak pro výpočet nadmořské výšky (uprav podle lokace)
const float SEA_LEVEL_HPA = 1013.25;

void initDisplay() {
    if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
        Serial.println("[ERROR] OLED nenalezen!");
        while (true); // Bez displeje nemá smysl pokračovat
    }
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1);
}

void showBootScreen() {
    display.clearDisplay();
    display.setCursor(20, 20);
    display.setTextSize(1);
    display.println("ESP32 Weather");
    display.setCursor(28, 35);
    display.println("Initializing...");
    display.display();
    delay(1500);
}

void initSensors() {
    ahtOK = aht.begin();
    if (!ahtOK) Serial.println("[WARN] AHT20 nenalezen na 0x38");

    bmpOK = bmp.begin(0x77);
    if (!bmpOK) Serial.println("[WARN] BMP280 nenalezen na 0x76");

    if (bmpOK) {
        bmp.setSampling(
            Adafruit_BMP280::MODE_NORMAL,
            Adafruit_BMP280::SAMPLING_X2,   // teplota
            Adafruit_BMP280::SAMPLING_X16,  // tlak
            Adafruit_BMP280::FILTER_X16,
            Adafruit_BMP280::STANDBY_MS_500
        );
    }
}

void readSensors() {
    if (ahtOK) {
        sensors_event_t hum, temp;
        aht.getEvent(&hum, &temp);
        tempAHT  = temp.temperature;
        humidity = hum.relative_humidity;
    }
    if (bmpOK) {
        tempBMP  = bmp.readTemperature();
        pressure = bmp.readPressure() / 100.0F; // Pa → hPa
        altitude = bmp.readAltitude(SEA_LEVEL_HPA);
    }
}

void updateDisplay() {
    display.clearDisplay();

    // --- Hlavička ---
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.println("=== Weather Sta. ===");

    if (ahtOK) {
        // Teplota — velký font
        display.setTextSize(2);
        display.setCursor(0, 12);
        display.print(tempAHT, 1);
        display.print((char)247); // stupen
        display.println("C");

        // Vlhkost
        display.setTextSize(1);
        display.setCursor(0, 30);
        display.print("RH: ");
        display.print(humidity, 1);
        display.println(" %");
    } else {
        display.setTextSize(1);
        display.setCursor(0, 12);
        display.println("AHT20: N/A");
    }

    if (bmpOK) {
        display.setCursor(0, 42);
        display.print("P:  ");
        display.print(pressure, 1);
        display.println(" hPa");

        display.setCursor(0, 54);
        display.print("Alt:");
        display.print(altitude, 0);
        display.println(" m");
    } else {
        display.setCursor(0, 42);
        display.println("BMP280: N/A");
    }

    display.display();
}

void printSerial() {
    Serial.println("--- Sensor Read ---");
    if (ahtOK) {
        Serial.printf("AHT20 -> T: %.2f C | RH: %.1f %%\n", tempAHT, humidity);
    }
    if (bmpOK) {
        Serial.printf("BMP280 -> T: %.2f C | P: %.1f hPa | Alt: %.1f m\n",
                      tempBMP, pressure, altitude);
    }
}

void setup() {
    Serial.begin(115200);
    Wire.begin(21, 22);

    initDisplay();
    showBootScreen();
    initSensors();
}

void loop() {
    unsigned long now = millis();
    if (now - lastRead >= READ_INTERVAL) {
        lastRead = now;
        readSensors();
        updateDisplay();
        printSerial();
    }
}