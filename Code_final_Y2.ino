code final : #include <Arduino.h>
#include <Wire.h>
#include <DHT.h>
#include <HX711.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include "esp_sleep.h"
#include "driver/gpio.h"

// ───────── LORA ─────────
#define LORA_RX_PIN 17
#define LORA_TX_PIN 16
#define LORA_BAUD   9600

// ───────── POWER ─────────
#define MOSFET_PIN 12

// ───────── BUZZER ─────────
#define BUZZER_PIN 4

// ───────── SENSORS ─────────
#define PIN_AM2302 23
#define PIN_DHT2   25

#define DT_PIN  32
#define SCK_PIN 33

#define ONE_WIRE_PIN 27
#define BATTERY_PIN 35

#define I2C_SDA     21
#define I2C_SCL     22
#define BH1750_ADDR 0x23

// ───────── HX711 CALIBRATION ─────────
#define FACTEUR_CALIBRATION 30006.9f
#define OFFSET_TARE         140894L

// ───────── BATTERY CALIBRATION (uPesy Low Power DevKit GPIO35) ─────────
#define BATTERY_FACTOR 1.36f

// ───────── TTN KEYS ─────────
#define DEVEUI "70B3D57ED0075CD3"
#define APPEUI "0000000000000000"
#define APPKEY "3C3B69C4A80B9BBD7EE479FF3CFCD21F"

// ───────── DEFAULT SLEEP ─────────
#define DEFAULT_SLEEP_MINUTES 10

HardwareSerial LoRaSerial(2);
DHT dht1(PIN_AM2302, DHT22);
DHT dht2(PIN_DHT2, DHT22);
HX711 balance;
OneWire oneWire(ONE_WIRE_PIN);
DallasTemperature ds18b20(&oneWire);

uint32_t sleepMinutes = DEFAULT_SLEEP_MINUTES;

// ───────── AT SEND ─────────
String sendAT(const String& cmd, uint32_t timeout = 4000)
{
  while (LoRaSerial.available()) LoRaSerial.read();

  Serial.print(">> ");
  Serial.println(cmd);
  LoRaSerial.println(cmd);

  String r;
  uint32_t t0 = millis();

  while (millis() - t0 < timeout)
  {
    while (LoRaSerial.available())
      r += (char)LoRaSerial.read();
    delay(5);
  }

  r.trim();
  Serial.print("<< ");
  Serial.println(r);
  return r;
}

// ───────── BUZZER ─────────
void beepStartup()
{
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  for (int i = 0; i < 2; i++)
  {
    digitalWrite(BUZZER_PIN, HIGH);
    delay(120);
    digitalWrite(BUZZER_PIN, LOW);
    delay(100);
  }
}

// ───────── LORA CONFIG ─────────
void configLoRa()
{
  sendAT("AT");
  sendAT("AT+RESET");
  delay(1200);

  sendAT("AT+MODE=LWOTAA");
  sendAT("AT+DR=EU868");
  sendAT("AT+CLASS=A");
  sendAT("AT+ADR=OFF");
  sendAT("AT+DR=DR0");
  sendAT("AT+POWER=22");
  sendAT("AT+PORT=1");

  sendAT("AT+ID=DevEui " + String(DEVEUI));
  sendAT("AT+ID=AppEui " + String(APPEUI));
  sendAT("AT+KEY=APPKEY " + String(APPKEY));
}

// ───────── JOIN ─────────
bool joinLoRa()
{
  Serial.println("[JOIN] start");

  while (LoRaSerial.available()) LoRaSerial.read();

  sendAT("AT+JOIN", 2000);

  uint32_t t0 = millis();
  while (millis() - t0 < 20000)
  {
    while (LoRaSerial.available())
    {
      char c = LoRaSerial.read();
      Serial.print(c);
    }
    delay(10);
  }
  Serial.println();

  String devaddr = sendAT("AT+ID=DevAddr", 2000);
  if (devaddr.indexOf("DevAddr") >= 0 && devaddr.indexOf("00000000") < 0)
  {
    Serial.println("[JOIN] OK (DevAddr set)");
    return true;
  }

  Serial.println("[JOIN] FAIL (DevAddr still 00000000 or no DevAddr)");
  return false;
}

// ───────── BH1750 ─────────
void bh1750Reset()
{
  Wire.beginTransmission(BH1750_ADDR);
  Wire.write(0x01);
  Wire.endTransmission();
  delay(20);

  Wire.beginTransmission(BH1750_ADDR);
  Wire.write(0x07);
  Wire.endTransmission();
  delay(20);
}

float readLux()
{
  bh1750Reset();

  Wire.beginTransmission(BH1750_ADDR);
  Wire.write(0x10);
  uint8_t err = Wire.endTransmission();

  if (err != 0)
  {
    Serial.print("Erreur I2C code ");
    Serial.println(err);
    return NAN;
  }

  delay(180);

  Wire.requestFrom(BH1750_ADDR, 2);
  if (Wire.available() >= 2)
  {
    uint16_t raw = ((uint16_t)Wire.read() << 8) | Wire.read();
    return raw / 1.2f;
  }

  Serial.println("No data from BH1750");
  return NAN;
}

// ───────── HX711 ─────────
float readWeight()
{
  if (!balance.is_ready()) return NAN;
  float poids_kg = balance.get_units(10);
  if (poids_kg < 0) poids_kg = 0;
  return poids_kg;
}

// ───────── BATTERY ─────────
float readBattery()
{
  pinMode(BATTERY_PIN, INPUT);
  int adc = analogRead(BATTERY_PIN);
  return BATTERY_FACTOR * (adc / 4095.0f) * 3.3f;
}

uint8_t batteryPercent(float v)
{
  if (isnan(v)) return 0;

  float pct = ((v - 3.2f) / (4.2f - 3.2f)) * 100.0f;
  if (pct < 0) pct = 0;
  if (pct > 100) pct = 100;
  return (uint8_t)roundf(pct);
}

// ───────── HELPERS ─────────
int16_t enc10(float v)
{
  if (isnan(v)) return 0;
  return (int16_t)roundf(v * 10.0f);
}

uint16_t encLux(float v)
{
  if (isnan(v) || v < 0) return 0;
  return (uint16_t)roundf(v);
}

uint16_t encBat(float v)
{
  if (isnan(v) || v < 0) return 0;
  return (uint16_t)roundf(v * 100.0f);
}

uint16_t encWeight(float v)
{
  if (isnan(v) || v < 0) return 0;
  return (uint16_t)roundf(v * 10.0f);
}

// ───────── DOWNLINK: 1 byte = minutes ─────────
void parseSleepDownlink(const String& r)
{
  int p = r.indexOf("RECVB");
  if (p < 0) return;

  int colon = r.indexOf(':', p);
  if (colon < 0) return;

  String tail = r.substring(colon + 1);
  tail.trim();
  if (tail.length() < 2) return;

  String hex = tail.substring(0, 2);
  long v = strtol(hex.c_str(), nullptr, 16);

  if (v >= 1 && v <= 255)
  {
    sleepMinutes = (uint32_t)v;
    Serial.printf("[DL] sleepMinutes=%lu\n", sleepMinutes);
  }
  else
  {
    Serial.println("[DL] invalid downlink value");
  }
}

void checkDownlink()
{
  String r = sendAT("AT+RECVB", 3000);
  parseSleepDownlink(r);
}

// ───────── SLEEP ─────────
void goToSleep()
{
  Serial.println("Preparing for deep sleep...");

  balance.power_down();
  delay(5);

  sendAT("AT+LOWPOWER", 1500);

  digitalWrite(MOSFET_PIN, LOW);
  delay(100);

  gpio_hold_dis((gpio_num_t)MOSFET_PIN);
  pinMode(MOSFET_PIN, OUTPUT);
  digitalWrite(MOSFET_PIN, LOW);
  gpio_hold_en((gpio_num_t)MOSFET_PIN);
  gpio_deep_sleep_hold_en();

  pinMode(BUZZER_PIN, INPUT);
  pinMode(LORA_RX_PIN, INPUT);
  pinMode(LORA_TX_PIN, INPUT);
  pinMode(DT_PIN, INPUT);
  pinMode(SCK_PIN, INPUT);
  pinMode(BATTERY_PIN, INPUT);
  pinMode(ONE_WIRE_PIN, INPUT);
  pinMode(PIN_AM2302, INPUT);
  pinMode(PIN_DHT2, INPUT);
  pinMode(I2C_SDA, INPUT);
  pinMode(I2C_SCL, INPUT);

  Wire.end();

  uint64_t sleepTimeUs = (uint64_t)sleepMinutes * 60ULL * 1000000ULL;

  Serial.printf("Going to deep sleep for %lu minutes...\n", sleepMinutes);
  Serial.flush();

  esp_sleep_enable_timer_wakeup(sleepTimeUs);
  esp_deep_sleep_start();
}

// ───────── SETUP ─────────
void setup()
{
  Serial.begin(115200);
  delay(500);

  beepStartup();
  Serial.println("\nBOOT LORA SYSTEM");

  sleepMinutes = DEFAULT_SLEEP_MINUTES;

  gpio_hold_dis((gpio_num_t)MOSFET_PIN);

  pinMode(MOSFET_PIN, OUTPUT);
  digitalWrite(MOSFET_PIN, HIGH);
  delay(1000);

  LoRaSerial.begin(LORA_BAUD, SERIAL_8N1, LORA_RX_PIN, LORA_TX_PIN);
  delay(1200);

  Wire.begin(I2C_SDA, I2C_SCL);
  analogReadResolution(12);

  dht1.begin();
  dht2.begin();
  ds18b20.begin();

  balance.begin(DT_PIN, SCK_PIN);
  balance.set_offset(OFFSET_TARE);
  balance.set_scale(FACTEUR_CALIBRATION);
  balance.power_up();
  delay(50);

  sendAT("AT");
  sendAT("AT+VER");

  configLoRa();

  if (!joinLoRa())
  {
    Serial.println("JOIN FAILED -> SLEEP");
    goToSleep();
  }

  float t1 = dht1.readTemperature();
  float h1 = dht1.readHumidity();
  float t2 = dht2.readTemperature();
  float h2 = dht2.readHumidity();

  ds18b20.requestTemperatures();
  int count = ds18b20.getDeviceCount();
  float ds0 = NAN, ds1 = NAN;
  if (count > 0) ds0 = ds18b20.getTempCByIndex(0);
  if (count > 1) ds1 = ds18b20.getTempCByIndex(1);

  float lux = readLux();
  float weight = readWeight();
  float bat = readBattery();
  uint8_t batPct = batteryPercent(bat);

  Serial.printf("AM2302 T=%.1f H=%.1f\n", t1, h1);
  Serial.printf("DHT22   T=%.1f H=%.1f\n", t2, h2);
  Serial.printf("DS0=%.1f DS1=%.1f count=%d\n", ds0, ds1, count);
  Serial.printf("LUX=%.1f\n", lux);
  Serial.printf("WEIGHT=%.2f kg\n", weight);
  Serial.printf("BAT=%.2f V (%u%%)\n", bat, batPct);

  int16_t  t1r = enc10(t1);
  int16_t  h1r = enc10(h1);
  int16_t  t2r = enc10(t2);
  int16_t  h2r = enc10(h2);
  int16_t  ds0r = enc10(ds0);
  int16_t  ds1r = enc10(ds1);
  uint16_t luxr = encLux(lux);
  uint16_t kgr  = encWeight(weight);
  uint16_t batr = encBat(bat);

  char payload[128];
  sprintf(payload,
          "%04X%04X%04X%04X%04X%04X%04X%04X%04X",
          (uint16_t)t1r,
          (uint16_t)h1r,
          (uint16_t)t2r,
          (uint16_t)h2r,
          (uint16_t)ds0r,
          (uint16_t)ds1r,
          luxr,
          kgr,
          batr);

  Serial.print("[PAYLOAD] ");
  Serial.println(payload);

  String res = sendAT(String("AT+MSGHEX=\"") + payload + "\"", 20000);

  if (res.indexOf("Done") >= 0)
    Serial.println("[TX] SUCCESS");
  else
    Serial.println("[TX] FAIL");

  checkDownlink();
  goToSleep();
}

void loop()
{
}