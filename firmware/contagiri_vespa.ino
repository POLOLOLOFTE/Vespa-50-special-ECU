// Contagiri Vespa 50 Special 2 tempi - ESP32 + OLED 1.3" 128x64 (I2C)
// Libreria necessaria: U8g2 (Library Manager)

#include <Arduino.h>
#include <Wire.h>
#include <U8g2lib.h>

// ---------- CONFIGURAZIONE ----------
#define PIN_SEGNALE     27      // uscita Vo del modulo (NON usare GPIO3/RX)
#define PIN_RESET_MAX    0      // tasto BOOT della scheda: azzera il massimo

const float    IMPULSI_PER_GIRO = 1.0f;     // 2 tempi monocilindrico: 1 scintilla per giro
const uint32_t DEBOUNCE_US      = 3000;     // ignora impulsi piu' vicini di 3 ms (disturbi)
const uint32_t TIMEOUT_US       = 1000000;  // nessun impulso per 1 s -> motore spento
const int      RPM_FONDOSCALA   = 12000;    // fondoscala della barra
const int      RPM_REDLINE      = 10000;    // da qui la barra lampeggia
const float    RPM_LIMITE_VALIDO = 15000;   // valori sopra questo sono considerati disturbi
const int      SEGMENTI         = 21;       // segmenti della barra

// Display SH1106 (tipico dei 1.3"). Se e' un SSD1306 usa invece:
// U8G2_SSD1306_128X64_NONAME_F_HW_I2C display(U8G2_R0, U8X8_PIN_NONE);
U8G2_SH1106_128X64_NONAME_F_HW_I2C display(U8G2_R0, U8X8_PIN_NONE);

// ---------- MISURA ----------
portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
volatile uint32_t ultimoImpulso = 0;
volatile uint32_t periodo = 0;   // microsecondi tra due impulsi

void IRAM_ATTR isrImpulso() {
  uint32_t ora = micros();
  portENTER_CRITICAL_ISR(&mux);
  uint32_t delta = ora - ultimoImpulso;
  if (delta >= DEBOUNCE_US) {
    periodo = delta;
    ultimoImpulso = ora;
  }
  portEXIT_CRITICAL_ISR(&mux);
}

float rpm = 0;
float rpmMax = 0;

// ---------- GRAFICA ----------
void scriviCentrato(const char* testo, int y) {
  int w = display.getStrWidth(testo);
  display.drawStr((128 - w) / 2, y, testo);
}

void splash() {
  display.clearBuffer();
  display.setFont(u8g2_font_logisoso16_tr);
  scriviCentrato("VESPA 50", 30);
  display.setFont(u8g2_font_6x10_tr);
  scriviCentrato("SPECIAL 75cc", 48);
  display.sendBuffer();
  delay(1500);
}

// Barra a segmenti crescenti (y da 0 a 13). Gli ultimi segmenti sono la zona rossa.
void disegnaBarra(float valore, bool flash) {
  int accesi = (int)(valore * SEGMENTI / RPM_FONDOSCALA + 0.5f);
  accesi = constrain(accesi, 0, SEGMENTI);
  int primoRosso = SEGMENTI * RPM_REDLINE / RPM_FONDOSCALA;

  for (int i = 0; i < SEGMENTI; i++) {
    int x = i * 6 + 1;
    int h = 5 + (i * 9) / (SEGMENTI - 1);   // altezza da 5 a 14 px
    int y = 14 - h;
    bool rosso = (i >= primoRosso);

    if (flash || i < accesi) display.drawBox(x, y, 5, h);       // acceso
    else if (rosso)          display.drawFrame(x, y, 5, h);     // zona rossa spenta (solo contorno)
    else                     display.drawBox(x, 12, 5, 2);      // spento: solo base
  }
}

// ---------- SETUP / LOOP ----------
void setup() {
  pinMode(PIN_SEGNALE, INPUT_PULLUP);
  pinMode(PIN_RESET_MAX, INPUT_PULLUP);
  // Fronte di discesa: va bene per uscita a transistor (collettore aperto).
  // Se non vedi giri, prova RISING.
  attachInterrupt(digitalPinToInterrupt(PIN_SEGNALE), isrImpulso, FALLING);

  display.begin();
  display.setBusClock(400000);
  splash();
}

void loop() {
  static uint32_t ultimoAggiornamento = 0;
  static bool flash = false;
  if (millis() - ultimoAggiornamento < 100) return;   // 10 aggiornamenti al secondo
  ultimoAggiornamento = millis();

  uint32_t p, ultimo;
  portENTER_CRITICAL(&mux);
  p = periodo;
  ultimo = ultimoImpulso;
  portEXIT_CRITICAL(&mux);

  uint32_t trascorso = micros() - ultimo;

  if (p == 0 || trascorso >= TIMEOUT_US) {
    rpm = 0;   // motore fermo
  } else {
    // Se dall'ultima scintilla e' passato piu' del periodo misurato, il motore sta rallentando:
    // uso il tempo trascorso come stima, cosi' i giri scendono subito.
    uint32_t periodoStimato = (trascorso > p) ? trascorso : p;
    float r = 60000000.0f / ((float)periodoStimato * IMPULSI_PER_GIRO);
    if (r < RPM_LIMITE_VALIDO) rpm = rpm * 0.7f + r * 0.3f;   // filtro anti-ballo
  }

  if (rpm > rpmMax) rpmMax = rpm;
  if (digitalRead(PIN_RESET_MAX) == LOW) rpmMax = 0;

  bool allarme = (rpm >= RPM_REDLINE);
  flash = allarme ? !flash : false;

  display.clearBuffer();
  disegnaBarra(rpm, flash);

  // Numero grande al centro
  char buf[8];
  snprintf(buf, sizeof(buf), "%d", (int)(rpm + 0.5f));
  display.setFont(u8g2_font_logisoso28_tn);
  scriviCentrato(buf, 50);

  // Riga in basso: etichetta e massimo
  display.setFont(u8g2_font_6x10_tr);
  display.drawStr(0, 63, "GIRI/MIN");
  char maxBuf[16];
  snprintf(maxBuf, sizeof(maxBuf), "MAX %d", (int)(rpmMax + 0.5f));
  display.drawStr(128 - display.getStrWidth(maxBuf), 63, maxBuf);

  display.sendBuffer();
}
