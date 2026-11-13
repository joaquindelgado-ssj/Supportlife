// Brazalete de Salud V2 - ESP32
// Sensores: MAX30102 (SpO2 + BPM), MPU-6050 (movimiento), DS18B20 (temperatura)
// Pines (segun el esquematico V2): SDA=IO21, SCL=IO22, DS18B20=IO4, BAT_SENSE=IO34, LED=IO2
// Envia una lectura cada ~5 s a  POST <SERVER_URL>  (JSON, header x-api-key)
#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include "MAX30105.h"
#include "spo2_algorithm.h"

// ---------------- CONFIGURACION (completar) ----------------
#define WIFI_SSID   "NOMBRE_DE_TU_WIFI"
#define WIFI_PASS   "CLAVE_DE_TU_WIFI"
#define SERVER_URL  "https://TU-PROYECTO.vercel.app/api/ingest"
#define API_KEY     "LA_MISMA_CLAVE_QUE_INGEST_KEY_EN_VERCEL"
#define DEVICE_ID   "brazalete-01"
// ------------------------------------------------------------

#define PIN_ONEWIRE  4
#define PIN_BAT      34
#define PIN_LED      2
#define MPU_ADDR     0x68
#define FINGER_IR_MIN 50000

MAX30105 sensor;
OneWire ow(PIN_ONEWIRE);
DallasTemperature ds(&ow);

static uint32_t irBuf[100], redBuf[100];
static bool haveMax = false, haveMpu = false;

bool mpuInit() {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x6B); Wire.write(0x00);          // sacar del sleep
  if (Wire.endTransmission() != 0) return false;
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x1C); Wire.write(0x00);          // +-2 g
  return Wire.endTransmission() == 0;
}

// devuelve |a| en g; ok=false si falla
float mpuMag(bool &ok) {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x3B);
  if (Wire.endTransmission(false) != 0 || Wire.requestFrom(MPU_ADDR, 6) != 6) { ok = false; return 1.0f; }
  int16_t x = (Wire.read() << 8) | Wire.read();
  int16_t y = (Wire.read() << 8) | Wire.read();
  int16_t z = (Wire.read() << 8) | Wire.read();
  ok = true;
  float gx = x / 16384.0f, gy = y / 16384.0f, gz = z / 16384.0f;
  return sqrtf(gx * gx + gy * gy + gz * gz);
}

void wifiConnect() {
  if (WiFi.status() == WL_CONNECTED) return;
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.print("WiFi");
  for (int i = 0; i < 40 && WiFi.status() != WL_CONNECTED; i++) { delay(500); Serial.print('.'); }
  Serial.println(WiFi.status() == WL_CONNECTED ? " OK" : " FALLO");
}

void setup() {
  Serial.begin(115200);
  pinMode(PIN_LED, OUTPUT);
  Wire.begin(21, 22);
  Wire.setClock(400000);
  haveMax = sensor.begin(Wire, I2C_SPEED_FAST);
  if (haveMax) {
    // ledBrightness, sampleAverage, ledMode(2=rojo+IR), sampleRate, pulseWidth, adcRange
    sensor.setup(60, 4, 2, 100, 411, 4096);
  } else Serial.println("MAX30102 no encontrado");
  haveMpu = mpuInit();
  if (!haveMpu) Serial.println("MPU-6050 no encontrado");
  ds.begin();
  ds.setWaitForConversion(false);
  analogReadResolution(12);
  wifiConnect();
}

void loop() {
  // ---- 1) ventana de 100 muestras (~4 s a 25 sps) ----
  float movSum = 0; int movN = 0;
  uint64_t irAcc = 0;
  ds.requestTemperatures();
  for (int i = 0; i < 100; i++) {
    if (haveMax) {
      while (!sensor.available()) sensor.check();
      redBuf[i] = sensor.getRed();
      irBuf[i]  = sensor.getIR();
      sensor.nextSample();
    } else { redBuf[i] = irBuf[i] = 0; delay(40); }
    irAcc += irBuf[i];
    if (haveMpu) {
      bool ok; float m = mpuMag(ok);
      if (ok) { movSum += fabsf(m - 1.0f); movN++; }
    }
  }
  bool finger = haveMax && (irAcc / 100) > FINGER_IR_MIN;

  int32_t spo2 = -999, hr = -999; int8_t spo2ok = 0, hrok = 0;
  if (finger) maxim_heart_rate_and_oxygen_saturation(irBuf, 100, redBuf, &spo2, &spo2ok, &hr, &hrok);

  float t = ds.getTempCByIndex(0);
  bool tempOk = t > -50 && t < 100 && t != DEVICE_DISCONNECTED_C;
  float motion = movN ? fminf(100.0f, movSum / movN / 0.5f * 100.0f) : -1;
  float vbat = analogReadMilliVolts(PIN_BAT) * 2.0f / 1000.0f;   // divisor 100k/100k

  // ---- 2) JSON ----
  char body[256];
  int n = snprintf(body, sizeof(body), "{\"device\":\"%s\",\"finger\":%s", DEVICE_ID, finger ? "true" : "false");
  if (finger && hrok && hr > 20 && hr < 250) n += snprintf(body + n, sizeof(body) - n, ",\"bpm\":%d", (int)hr);
  if (finger && spo2ok && spo2 > 50 && spo2 <= 100) n += snprintf(body + n, sizeof(body) - n, ",\"spo2\":%d", (int)spo2);
  if (tempOk) n += snprintf(body + n, sizeof(body) - n, ",\"temp\":%.1f", t);
  if (motion >= 0) n += snprintf(body + n, sizeof(body) - n, ",\"motion\":%.0f", motion);
  n += snprintf(body + n, sizeof(body) - n, ",\"bat\":%.2f}", vbat);
  Serial.println(body);

  // ---- 3) enviar ----
  wifiConnect();
  if (WiFi.status() == WL_CONNECTED) {
    WiFiClientSecure client;
    client.setInsecure();                       // demo escolar: sin validar el certificado
    HTTPClient http;
    if (http.begin(client, SERVER_URL)) {
      http.addHeader("Content-Type", "application/json");
      http.addHeader("x-api-key", API_KEY);
      int code = http.POST((uint8_t *)body, n);
      Serial.printf("HTTP %d\n", code);
      digitalWrite(PIN_LED, code == 201 ? HIGH : LOW);
      http.end();
    }
  }
  if (haveMax) sensor.clearFIFO();
}
