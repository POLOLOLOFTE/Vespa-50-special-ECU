// Vespa 50 Special 2-stroke - tachometer + speedometer
// Original ESP32 (needs Bluetooth Classic)

#include <Arduino.h>
#include <Wire.h>
#include <U8g2lib.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include "BluetoothSerial.h"

#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled - this sketch needs the original ESP32 (Bluetooth Classic)
#endif
#if !defined(CONFIG_BT_SPP_ENABLED)
#error Serial Port Profile (SPP) is not available - it only works on the original ESP32
#endif

// ---------- CONFIGURATION ----------
#define SIGNAL_PIN   27          // optocoupler output (Vo)
#define BUTTON_PIN   25          // push button to GND

const char*    BT_NAME           = "Vespa-RPM";

// RPM
const float    PULSES_PER_REV    = 1.0f;      // 2-stroke single cylinder: 1 spark per revolution
const uint32_t DEBOUNCE_US       = 3000;      // ignore pulses closer than 3 ms (noise)
const uint32_t RPM_TIMEOUT_US    = 1000000;   // no pulse for 1 s -> engine stopped
const float    RPM_MAX_VALID     = 15000.0f;  // anything above is treated as noise
const float    RPM_FULL_SCALE    = 12000.0f;  // bar full scale
const float    RPM_REDLINE       = 10000.0f;  // bar flashes from here

// Speed
const float    SPEED_FULL_SCALE  = 100.0f;    // km/h, bar full scale
const float    SPEED_WARNING     = 70.0f;     // km/h, bar flashes from here (set 45 for city limit)
const float    SPEED_DEADBAND    = 2.0f;      // GPS jitter: below this, show 0
const uint32_t SPEED_TIMEOUT_MS  = 3000;      // no data for 3 s -> show "--"

// UI
const uint32_t BUTTON_DEBOUNCE_MS = 30;
const uint32_t LONG_PRESS_MS      = 800;
const uint32_t FRAME_MS           = 100;      // display refresh period
const int      SEGMENTS           = 21;       // bar segments

// SH1106 display (typical for 1.3"). If it is an SSD1306 use this instead:
// U8G2_SSD1306_128X64_NONAME_F_HW_I2C display(U8G2_R0, U8X8_PIN_NONE);
U8G2_SH1106_128X64_NONAME_F_HW_I2C display(U8G2_R0, U8X8_PIN_NONE);

BluetoothSerial SerialBT;

// ---------- STATE ----------
enum View { VIEW_RPM, VIEW_SPEED };
View currentView = VIEW_RPM;

// RPM measurement (written by the interrupt)
portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
volatile uint32_t lastPulseUs = 0;
volatile uint32_t pulsePeriodUs = 0;   // microseconds between the last two pulses

float rpm = 0;
float maxRpm = 0;

// Speed (received over Bluetooth)
float    speedKmh = 0;
float    maxSpeed = 0;
bool     hasSpeed = false;
uint32_t lastSpeedMs = 0;

volatile bool btConnected = false;

char   lineBuf[96];
size_t lineLen = 0;

// ---------- RPM ----------
void IRAM_ATTR onPulse() {
  uint32_t now = micros();
  portENTER_CRITICAL_ISR(&mux);
  uint32_t delta = now - lastPulseUs;
  if (delta >= DEBOUNCE_US) {
    pulsePeriodUs = delta;
    lastPulseUs = now;
  }
  portEXIT_CRITICAL_ISR(&mux);
}

void updateRpm() {
  uint32_t period, last;
  portENTER_CRITICAL(&mux);
  period = pulsePeriodUs;
  last = lastPulseUs;
  portEXIT_CRITICAL(&mux);

  uint32_t elapsed = micros() - last;

  if (period == 0 || elapsed >= RPM_TIMEOUT_US) {
    rpm = 0;   // engine stopped
    return;
  }

  // If much more time than the last period has passed, the engine is slowing down:
  // use the elapsed time as the estimate so the number drops immediately.
  uint32_t estimated = (elapsed > period + period / 2) ? elapsed : period;
  float r = 60000000.0f / ((float)estimated * PULSES_PER_REV);

  if (r < RPM_MAX_VALID) rpm = rpm * 0.7f + r * 0.3f;   // smoothing filter
  if (rpm > maxRpm) maxRpm = rpm;
}

// ---------- SPEED (Bluetooth input) ----------
void btCallback(esp_spp_cb_event_t event, esp_spp_cb_param_t* param) {
  if (event == ESP_SPP_SRV_OPEN_EVT)      btConnected = true;
  else if (event == ESP_SPP_CLOSE_EVT)    btConnected = false;
}

void setSpeed(float kmh) {
  if (kmh < 0 || kmh > 250) return;          // reject nonsense
  if (kmh < SPEED_DEADBAND) kmh = 0;
  speedKmh = kmh;
  hasSpeed = true;
  lastSpeedMs = millis();
  if (kmh > maxSpeed) maxSpeed = kmh;
}

bool nmeaField(const char* s, int index, char* out, size_t outSize) {
  int field = 0;
  while (*s) {
    if (field == index) {
      size_t n = 0;
      while (*s && *s != ',' && *s != '*' && n < outSize - 1) out[n++] = *s++;
      out[n] = '\0';
      return n > 0;
    }
    if (*s == ',') field++;
    s++;
  }
  return false;
}

bool nmeaChecksumOk(const char* s) {
  const char* star = strchr(s, '*');
  if (!star) return true;                    // no checksum present: accept
  uint8_t sum = 0;
  for (const char* p = s + 1; p < star; p++) sum ^= (uint8_t)*p;
  return sum == (uint8_t)strtol(star + 1, nullptr, 16);
}

void parseLine(const char* line) {
  char field[16];

  if (line[0] == '$') {
    // NMEA sentence
    if (strlen(line) < 7 || !nmeaChecksumOk(line)) return;

    if (strncmp(line + 3, "RMC", 3) == 0) {
      // $GPRMC,time,status,lat,N,lon,E,speed(knots),course,...
      if (!nmeaField(line, 2, field, sizeof(field)) || field[0] != 'A') {
        hasSpeed = false;                    // no valid fix
        return;
      }
      if (nmeaField(line, 7, field, sizeof(field))) setSpeed((float)atof(field) * 1.852f);

    } else if (strncmp(line + 3, "VTG", 3) == 0) {
      // $GPVTG,course,T,course,M,speed(knots),N,speed(km/h),K,...
      if (nmeaField(line, 7, field, sizeof(field))) setSpeed((float)atof(field));
    }

  } else if (isdigit((uint8_t)line[0]) || line[0] == '.') {
    // plain number in km/h
    setSpeed((float)atof(line));
  }
}

void handleBluetooth() {
  while (SerialBT.available()) {
    char c = (char)SerialBT.read();
    if (c == '\n' || c == '\r') {
      if (lineLen > 0) {
        lineBuf[lineLen] = '\0';
        parseLine(lineBuf);
        lineLen = 0;
      }
    } else if (lineLen < sizeof(lineBuf) - 1) {
      lineBuf[lineLen++] = c;
    } else {
      lineLen = 0;                           // line too long: discard it
    }
  }
}

// ---------- BUTTON ----------
void handleButton() {
  static bool lastRaw = HIGH;
  static bool stable = HIGH;
  static uint32_t lastChange = 0;
  static uint32_t pressedAt = 0;

  bool raw = digitalRead(BUTTON_PIN);
  if (raw != lastRaw) {
    lastRaw = raw;
    lastChange = millis();
  }

  if (millis() - lastChange > BUTTON_DEBOUNCE_MS && raw != stable) {
    stable = raw;
    if (stable == LOW) {
      pressedAt = millis();                  // pressed
    } else {                                 // released
      if (millis() - pressedAt >= LONG_PRESS_MS) {
        maxRpm = 0;
        maxSpeed = 0;
      } else {
        currentView = (currentView == VIEW_RPM) ? VIEW_SPEED : VIEW_RPM;
      }
    }
  }
}

// ---------- GRAPHICS ----------
void drawCentered(const char* text, int y) {
  int w = display.getStrWidth(text);
  display.drawStr((128 - w) / 2, y, text);
}

void splash() {
  display.clearBuffer();
  display.setFont(u8g2_font_logisoso16_tr);
  drawCentered("VESPA 50", 26);              //!!!!change here if you want a custom start up!!!!
  display.setFont(u8g2_font_6x10_tr);
  drawCentered("SPECIAL 75cc", 42);          //!!!!and here!!!!
  char bt[32];
  snprintf(bt, sizeof(bt), "BT: %s", BT_NAME);
  drawCentered(bt, 58);
  display.sendBuffer();
  delay(1800);
}

// Bar with growing segments (y from 0 to 13). The last segments are the red zone.
void drawBar(float value, float fullScale, float redStart, bool flash) {
  int lit = constrain((int)(value * SEGMENTS / fullScale + 0.5f), 0, SEGMENTS);
  int firstRed = (int)(SEGMENTS * redStart / fullScale);

  for (int i = 0; i < SEGMENTS; i++) {
    int x = i * 6 + 1;
    int h = 5 + (i * 9) / (SEGMENTS - 1);    // height from 5 to 14 px
    int y = 14 - h;
    bool red = (i >= firstRed);

    if (flash || i < lit) display.drawBox(x, y, 5, h);        // lit
    else if (red)         display.drawFrame(x, y, 5, h);      // unlit red zone (outline only)
    else                  display.drawBox(x, 12, 5, 2);       // unlit: base only
  }
}

void drawDashes() {   // "--" when there is no speed data
  display.drawBox(40, 34, 20, 5);
  display.drawBox(68, 34, 20, 5);
}

// ---------- SETUP / LOOP ----------
void setup() {
  Serial.begin(115200);

  pinMode(SIGNAL_PIN, INPUT_PULLUP);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(SIGNAL_PIN), onPulse, FALLING);

  SerialBT.register_callback(btCallback);
  if (!SerialBT.begin(BT_NAME)) Serial.println("Bluetooth start failed");

  display.begin();
  display.setBusClock(400000);
  splash();
}

void loop() {
  handleButton();
  handleBluetooth();

  static uint32_t lastFrame = 0;
  static uint32_t lastPrint = 0;
  static bool flash = false;

  if (millis() - lastFrame < FRAME_MS) {
    delay(1);
    return;
  }
  lastFrame = millis();

  updateRpm();
  bool speedFresh = hasSpeed && (millis() - lastSpeedMs < SPEED_TIMEOUT_MS);

  bool showSpeed     = (currentView == VIEW_SPEED);
  float value        = showSpeed ? speedKmh : rpm;
  float fullScale    = showSpeed ? SPEED_FULL_SCALE : RPM_FULL_SCALE;
  float warningLevel = showSpeed ? SPEED_WARNING : RPM_REDLINE;
  float maxValue     = showSpeed ? maxSpeed : maxRpm;
  bool  valid        = showSpeed ? speedFresh : true;
  const char* label  = showSpeed ? "KM/H" : "RPM";

  flash = (valid && value >= warningLevel) ? !flash : false;

  display.clearBuffer();
  drawBar(valid ? value : 0, fullScale, warningLevel, flash);

  // Big number in the middle
  if (valid) {
    char buf[8];
    snprintf(buf, sizeof(buf), "%d", (int)(value + 0.5f));
    display.setFont(u8g2_font_logisoso28_tn);
    drawCentered(buf, 50);
  } else {
    drawDashes();
  }

  // Bottom row: label, Bluetooth indicator, max value
  display.setFont(u8g2_font_6x10_tr);
  display.drawStr(0, 63, label);
  if (btConnected) drawCentered("BT", 63);
  char maxBuf[16];
  snprintf(maxBuf, sizeof(maxBuf), "MAX %d", (int)(maxValue + 0.5f));
  display.drawStr(128 - display.getStrWidth(maxBuf), 63, maxBuf);

  display.sendBuffer();

  // Debug over USB serial, once per second
  if (millis() - lastPrint >= 1000) {
    lastPrint = millis();
    Serial.printf("RPM %d | speed %.1f km/h (%s) | BT %s\n",
                  (int)(rpm + 0.5f), speedKmh,
                  speedFresh ? "ok" : "no data",
                  btConnected ? "connected" : "waiting");
  }
}
