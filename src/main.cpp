#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_AHTX0.h>
#include <Adafruit_BMP280.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
#define OLED_ADDR     0x3C

// Graf
#define GRAPH_WIDTH   128
#define GRAPH_HEIGHT  36        // spodních 36px = graf
#define GRAPH_Y_OFFSET 28       // kde graf začíná (od vrchu)
#define GRAPH_POINTS  GRAPH_WIDTH

Adafruit_AHTX0 aht;
Adafruit_BMP280 bmp;
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

bool ahtOK = false;
bool bmpOK = false;

const unsigned long READ_INTERVAL = 2000;
unsigned long lastRead = 0;

float tempAHT = 0, humidity = 0;
float tempBMP = 0, pressure = 0;

// Kruhový buffer pro graf
float graphBuf[GRAPH_POINTS];
int   graphHead = 0;
int   graphCount = 0;

// Rozsah grafu — uprav podle očekávané teploty
float GRAPH_TEMP_MIN = 15.0;
float GRAPH_TEMP_MAX = 40.0;

void initDisplay() {
    if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
        Serial.println("[ERROR] OLED nenalezen!");
        while (true);
    }
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
}

void initSensors() {
    ahtOK = aht.begin();
    if (!ahtOK) Serial.println("[WARN] AHT20 nenalezen");

    bmpOK = bmp.begin(0x77);
    if (!bmpOK) Serial.println("[WARN] BMP280 nenalezen");

    if (bmpOK) {
        bmp.setSampling(
            Adafruit_BMP280::MODE_NORMAL,
            Adafruit_BMP280::SAMPLING_X2,
            Adafruit_BMP280::SAMPLING_X16,
            Adafruit_BMP280::FILTER_X16,
            Adafruit_BMP280::STANDBY_MS_500
        );
    }

    // Inicializuj buffer
    for (int i = 0; i < GRAPH_POINTS; i++) graphBuf[i] = 0;
}

void pushGraph(float val) {
    graphBuf[graphHead] = val;
    graphHead = (graphHead + 1) % GRAPH_POINTS;
    if (graphCount < GRAPH_POINTS) graphCount++;
}

// Automaticky přizpůsob rozsah grafu podle dat v bufferu
void autoScale() {
    if (graphCount < 2) return;
    float mn = 9999, mx = -9999;
    for (int i = 0; i < graphCount; i++) {
        int idx = (graphHead - graphCount + i + GRAPH_POINTS) % GRAPH_POINTS;
        mn = min(mn, graphBuf[idx]);
        mx = max(mx, graphBuf[idx]);
    }
    float margin = max(1.0f, (mx - mn) * 0.1f);
    GRAPH_TEMP_MIN = mn - margin;
    GRAPH_TEMP_MAX = mx + margin;
}

void drawGraph() {
    autoScale();

    // Osa Y — krajní hodnoty
    display.setTextSize(1);
    display.setCursor(0, GRAPH_Y_OFFSET);
    display.print((int)GRAPH_TEMP_MAX);

    display.setCursor(0, SCREEN_HEIGHT - 8);
    display.print((int)GRAPH_TEMP_MIN);

    // Vodorovná dělící čára
    display.drawFastHLine(0, GRAPH_Y_OFFSET - 1, SCREEN_WIDTH, SSD1306_WHITE);

    // Vykresli body grafu
    int xStart = 18; // odsazení kvůli Y popiskům
    int graphW = SCREEN_WIDTH - xStart;

    for (int i = 0; i < min(graphCount, graphW); i++) {
        int idx = (graphHead - min(graphCount, graphW) + i + GRAPH_POINTS) % GRAPH_POINTS;
        float val = graphBuf[idx];

        float norm = (val - GRAPH_TEMP_MIN) / (GRAPH_TEMP_MAX - GRAPH_TEMP_MIN);
        norm = constrain(norm, 0.0f, 1.0f);

        int x = xStart + i;
        int y = SCREEN_HEIGHT - 1 - (int)(norm * (GRAPH_HEIGHT - 1));

        display.drawPixel(x, y, SSD1306_WHITE);
    }
}

void updateDisplay() {
    display.clearDisplay();

    // --- Horní panel: aktuální hodnoty ---
    display.setTextSize(1);
    display.setCursor(0, 0);

    if (ahtOK) {
        display.print("T:");
        display.print(tempAHT, 1);
        display.print("C ");
        display.print("RH:");
        display.print(humidity, 0);
        display.print("%");
    } else {
        display.print("AHT: N/A");
    }

    display.setCursor(0, 10);
    if (bmpOK) {
        display.print("BMP:");
        display.print(tempBMP, 1);
        display.print("C P:");
        display.print(pressure, 0);
        display.print("hPa");
    } else {
        display.print("BMP: N/A");
    }

    // --- Graf ---
    drawGraph();

    display.display();
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
        pressure = bmp.readPressure() / 100.0F;
    }

    // Do grafu jde AHT20, fallback BMP280
    float graphVal = ahtOK ? tempAHT : (bmpOK ? tempBMP : 0);
    pushGraph(graphVal);
}

void setup() {
    Serial.begin(115200);
    Wire.begin(21, 22);
    initDisplay();
    initSensors();
}

void loop() {
    unsigned long now = millis();
    if (now - lastRead >= READ_INTERVAL) {
        lastRead = now;
        readSensors();
        updateDisplay();
    }
}