#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_AHTX0.h>
#include <Adafruit_BMP280.h>
#include <Adafruit_SSD1306.h>
#include <IRremoteESP8266.h>
#include <IRsend.h>

// ── Piny ─────────────────────────────────────────────
#define IR_PIN        15
#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
#define OLED_ADDR     0x3C

// ── IR kódy (NEC, 32bit: usercode << 16 | datacode) ─
#define IR_ADDR       0x807F
#define CMD_POWER     0x9C63
#define CMD_SWING     0x926D
#define CMD_TIMER     0x9F60
#define CMD_TEMP_DOWN 0x956A
#define CMD_TEMP_UP   0x946B
#define CMD_FAN       0x9669
#define CMD_SLEEP     0x9B64

// ── Teplotní práh ────────────────────────────────────
const float TEMP_ON  = 26.0;
const float TEMP_OFF = 22.0;

// ── Graf ─────────────────────────────────────────────
#define GRAPH_POINTS  128
#define GRAPH_HEIGHT  36
#define GRAPH_Y_OFFSET 28

// ── Objekty ──────────────────────────────────────────
Adafruit_AHTX0    aht;
Adafruit_BMP280   bmp;
Adafruit_SSD1306  display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
IRsend            irsend(IR_PIN);

// ── Stav ─────────────────────────────────────────────
bool ahtOK = false;
bool bmpOK = false;
bool climaON = false;

float tempAHT = 0, humidity = 0;
float tempBMP = 0, pressure = 0;

float graphBuf[GRAPH_POINTS];
int   graphHead  = 0;
int   graphCount = 0;
float GRAPH_TEMP_MIN = 15.0;
float GRAPH_TEMP_MAX = 40.0;

const unsigned long READ_INTERVAL = 2000;
unsigned long lastRead = 0;

// ── IR helper ────────────────────────────────────────
void sendNEC(uint16_t addr, uint16_t cmd) {
    uint32_t raw = ((uint32_t)addr << 16) | cmd;
    irsend.sendNEC(raw, 32);
    Serial.printf("[IR] sent 0x%08X\n", raw);
}

// ── Klimatizace ──────────────────────────────────────
void climaOn() {
    if (!climaON) {
        sendNEC(IR_ADDR, CMD_POWER);
        climaON = true;
        Serial.println("[CLIMA] Zapnuto");
    }
}

void climaOff() {
    if (climaON) {
        sendNEC(IR_ADDR, CMD_POWER);
        climaON = false;
        Serial.println("[CLIMA] Vypnuto");
    }
}

void checkClima() {
    float t = ahtOK ? tempAHT : (bmpOK ? tempBMP : NAN);
    if (isnan(t)) return;

    if (!climaON && t >= TEMP_ON) climaOn();
    else if (climaON && t <= TEMP_OFF) climaOff();
}

// ── Graf ─────────────────────────────────────────────
void pushGraph(float val) {
    graphBuf[graphHead] = val;
    graphHead = (graphHead + 1) % GRAPH_POINTS;
    if (graphCount < GRAPH_POINTS) graphCount++;
}

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

    display.setTextSize(1);
    display.setCursor(0, GRAPH_Y_OFFSET);
    display.print((int)GRAPH_TEMP_MAX);
    display.setCursor(0, SCREEN_HEIGHT - 8);
    display.print((int)GRAPH_TEMP_MIN);

    display.drawFastHLine(0, GRAPH_Y_OFFSET - 1, SCREEN_WIDTH, SSD1306_WHITE);

    // Práhové čáry
    auto drawThreshLine = [](float thresh, float mn, float mx) {
        if (thresh < mn || thresh > mx) return;
        float norm = (thresh - mn) / (mx - mn);
        int y = SCREEN_HEIGHT - 1 - (int)(norm * (GRAPH_HEIGHT - 1));
        for (int x = 18; x < SCREEN_WIDTH; x += 3)
            display.drawPixel(x, y, SSD1306_WHITE); // tečkovaná čára
    };

    drawThreshLine(TEMP_ON,  GRAPH_TEMP_MIN, GRAPH_TEMP_MAX);
    drawThreshLine(TEMP_OFF, GRAPH_TEMP_MIN, GRAPH_TEMP_MAX);

    int xStart = 18;
    int graphW  = SCREEN_WIDTH - xStart;

    for (int i = 0; i < min(graphCount, graphW); i++) {
        int idx = (graphHead - min(graphCount, graphW) + i + GRAPH_POINTS) % GRAPH_POINTS;
        float val = graphBuf[idx];
        float norm = constrain((val - GRAPH_TEMP_MIN) / (GRAPH_TEMP_MAX - GRAPH_TEMP_MIN), 0.0f, 1.0f);
        int x = xStart + i;
        int y = SCREEN_HEIGHT - 1 - (int)(norm * (GRAPH_HEIGHT - 1));
        display.drawPixel(x, y, SSD1306_WHITE);
    }
}

// ── Display ──────────────────────────────────────────
void updateDisplay() {
    display.clearDisplay();

    display.setTextSize(1);
    display.setCursor(0, 0);
    if (ahtOK) {
        display.print("T:");
        display.print(tempAHT, 1);
        display.print("C RH:");
        display.print(humidity, 0);
        display.print("%");
    } else {
        display.print("AHT: N/A");
    }

    display.setCursor(0, 10);
    if (bmpOK) {
        display.print("P:");
        display.print(pressure, 0);
        display.print("hPa ");
    }
    // Stav klimatizace vpravo
    display.setCursor(84, 10);
    display.print("AC:");
    display.print(climaON ? "ON " : "OFF");

    drawGraph();
    display.display();
}

// ── Senzory ──────────────────────────────────────────
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
    float graphVal = ahtOK ? tempAHT : (bmpOK ? tempBMP : 0);
    pushGraph(graphVal);
}

// ── Setup / Loop ─────────────────────────────────────
void setup() {
    Serial.begin(115200);
    Wire.begin(21, 22);

    if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
        Serial.println("[ERROR] OLED nenalezen");
        while (true);
    }

    ahtOK = aht.begin();
    bmpOK = bmp.begin(0x77);
    if (bmpOK) {
        bmp.setSampling(
            Adafruit_BMP280::MODE_NORMAL,
            Adafruit_BMP280::SAMPLING_X2,
            Adafruit_BMP280::SAMPLING_X16,
            Adafruit_BMP280::FILTER_X16,
            Adafruit_BMP280::STANDBY_MS_500
        );
    }

    irsend.begin();
    for (int i = 0; i < GRAPH_POINTS; i++) graphBuf[i] = 0;

    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1);
    display.setCursor(20, 25);
    display.println("ESP32 ClimaSta.");
    display.display();
    delay(1500);
}

void loop() {
    unsigned long now = millis();
    if (now - lastRead >= READ_INTERVAL) {
        lastRead = now;
        readSensors();
        checkClima();
        updateDisplay();
    }
}