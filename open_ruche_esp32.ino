/**
 * ================================================================
 *  ESP32 WROOM — DHT22 + LoRa-E5 (Seeed) OTAA TTN + Deep Sleep
 * ================================================================
 *  CÂBLAGE :
 *  LoRa-E5 TX  →  GPIO 16 (RX2 de l'ESP32)
 *  LoRa-E5 RX  →  GPIO 17 (TX2 de l'ESP32)
 *  LoRa-E5 VCC →  3.3V
 *  LoRa-E5 GND →  GND
 *  DHT22 DATA  →  GPIO 23  (+ pull-up 10kΩ vers 3.3V)
 * ================================================================
 */

#include <Arduino.h>
#include <DHT.h>
#include "esp_sleep.h"
#include "esp_task_wdt.h"

// ── Pins ──────────────────────────────────────────────────────
#define LORA_RX_PIN   16
#define LORA_TX_PIN   17
#define LORA_BAUD     9600
#define PIN_DHT       23
#define DHTTYPE       DHT22

// ── Deep Sleep 1 min (test) — remettre 10 en production ───────
#define SLEEP_MINUTES   1
#define SLEEP_US        (SLEEP_MINUTES * 60ULL * 1000000ULL)

// ── Clés TTN v3 ───────────────────────────────────────────────
#define DEVEUI  "70B3D57ED0075CD3"
#define APPEUI  "0000000000000000"
#define APPKEY  "3C3B69C4A80B9BBD7EE479FF3CFCD21F"

// ── RTC memory (survit au deep sleep) ─────────────────────────
RTC_DATA_ATTR bool     rtcJoined = false;
RTC_DATA_ATTR uint32_t wakeCount = 0;

// ── Objets ────────────────────────────────────────────────────
HardwareSerial LoRaSerial(2);
DHT dht(PIN_DHT, DHTTYPE);

// ================================================================
//  Envoyer commande AT et lire réponse
// ================================================================
String sendAT(const char* cmd, uint32_t timeout = 3000) {
    while (LoRaSerial.available()) LoRaSerial.read();

    Serial.printf("[AT] >> %s\n", cmd);
    LoRaSerial.println(cmd);

    String response = "";
    uint32_t t = millis();

    while (millis() - t < timeout) {
        esp_task_wdt_reset();
        while (LoRaSerial.available()) {
            response += (char)LoRaSerial.read();
        }
        if (response.indexOf("+ACK")              >= 0 ||
            response.indexOf("+MSGHEX: Done")      >= 0 ||
            response.indexOf("Network joined")     >= 0 ||
            response.indexOf("Already joined")     >= 0 ||
            response.indexOf("+JOIN: Join failed") >= 0 ||
            response.indexOf("+AT: OK")            >= 0 ||
            response.indexOf("ERROR")              >= 0) {
            break;
        }
        delay(10);
    }

    response.trim();
    Serial.printf("[AT] << %s\n", response.c_str());
    return response;
}

// ================================================================
//  Raison du réveil
// ================================================================
void printWakeReason() {
    esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();
    switch (cause) {
        case ESP_SLEEP_WAKEUP_TIMER:
            Serial.println("  Cause réveil : TIMER (normal)");
            break;
        case ESP_SLEEP_WAKEUP_UNDEFINED:
            Serial.println("  Cause réveil : POWER ON / RESET (premier boot)");
            break;
        default:
            Serial.printf("  Cause réveil : autre (%d)\n", cause);
            break;
    }
}

// ================================================================
//  Deep sleep
// ================================================================
void goToSleep() {
    Serial.printf("\n[SLEEP] Entrée deep sleep — réveil dans %d min\n",
                  SLEEP_MINUTES);
    Serial.flush();
    delay(200);
    esp_task_wdt_delete(NULL);
    esp_sleep_enable_timer_wakeup(SLEEP_US);
    esp_deep_sleep_start();
}

// ================================================================
//  SETUP — exécuté à chaque réveil
// ================================================================
void setup() {

    // ── Watchdog IDF v5.x ─────────────────────────────────────
    const esp_task_wdt_config_t wdt_config = {
        .timeout_ms     = 30000,
        .idle_core_mask = 0,
        .trigger_panic  = false
    };
    esp_task_wdt_reconfigure(&wdt_config);
    esp_task_wdt_add(NULL);

    Serial.begin(115200);
    delay(500);

    wakeCount++;
    Serial.println("\n╔══════════════════════════════════════╗");
    Serial.println("║  ESP32 WROOM + LoRa-E5 + DHT22 TTN  ║");
    Serial.println("╚══════════════════════════════════════╝");
    Serial.printf("  Réveil #%lu\n", wakeCount);
    printWakeReason();
    Serial.printf("  Session OTAA : %s\n",
                  rtcJoined ? "sauvegardée" : "à établir");

    // ── Init UART2 → LoRa-E5 ─────────────────────────────────
    LoRaSerial.begin(LORA_BAUD, SERIAL_8N1, LORA_RX_PIN, LORA_TX_PIN);
    delay(2000);  // ← 2s pour laisser le LoRa-E5 booter complètement
    esp_task_wdt_reset();

    // ── Lecture DHT22 — 5 tentatives ──────────────────────────
    dht.begin();
    delay(2000);
    esp_task_wdt_reset();

    float temperature = NAN;
    float humidity    = NAN;

    for (int i = 0; i < 5; i++) {
        temperature = dht.readTemperature();
        humidity    = dht.readHumidity();
        if (!isnan(temperature) && !isnan(humidity)) break;
        Serial.printf("[DHT22] Tentative %d/5 échouée...\n", i + 1);
        delay(500);
        esp_task_wdt_reset();
    }

    if (isnan(temperature) || isnan(humidity)) {
        Serial.println("[DHT22] ERREUR : 5 tentatives échouées !");
        Serial.println("  → Vérifier : VCC=3.3V, DATA=GPIO23, pull-up 10kΩ");
        goToSleep();
    }

    Serial.printf("[DHT22] Température : %.1f °C\n", temperature);
    Serial.printf("[DHT22] Humidité    : %.1f %%\n", humidity);

    // ── Encodage payload 4 octets little-endian ───────────────
    int16_t  tempRaw = (int16_t)(temperature * 10.0f);
    uint16_t humRaw  = (uint16_t)(humidity   * 10.0f);

    char payload[9];
    snprintf(payload, sizeof(payload), "%02X%02X%02X%02X",
             (uint8_t)( tempRaw        & 0xFF),
             (uint8_t)((tempRaw >> 8)  & 0xFF),
             (uint8_t)( humRaw         & 0xFF),
             (uint8_t)((humRaw  >> 8)  & 0xFF));

    Serial.printf("[Payload] HEX : %s\n", payload);

    // ── Test AT — 5 tentatives ────────────────────────────────
    Serial.println("[LoRa-E5] Test communication AT...");
    String resp = "";
    bool moduleOK = false;

    for (int i = 0; i < 5; i++) {
        esp_task_wdt_reset();
        resp = sendAT("AT", 2000);
        if (resp.indexOf("+AT: OK") >= 0) {
            moduleOK = true;
            break;
        }
        Serial.printf("[LoRa-E5] Tentative %d/5 — pas de réponse...\n", i + 1);
        delay(1000);
    }

    if (!moduleOK) {
        Serial.println("[LoRa-E5] PAS DE REPONSE AT après 5 tentatives !");
        Serial.println("  → Vérifier : TX(E5)→GPIO16, RX(E5)→GPIO17");
        Serial.println("  → Vérifier : alim 3.3V stable, antenne connectée");
        goToSleep();
    }

    Serial.println("[LoRa-E5] Module OK");
    esp_task_wdt_reset();

    // ── OTAA : join ou vérification session ───────────────────
    if (!rtcJoined) {

        Serial.println("[LoRa-E5] === Premier join OTAA ===");

        sendAT("AT+DR=EU868");    delay(200); esp_task_wdt_reset();
        sendAT("AT+MODE=LWOTAA"); delay(200); esp_task_wdt_reset();

        sendAT(("AT+ID=DevEui,\""  + String(DEVEUI) + "\"").c_str()); delay(200);
        sendAT(("AT+ID=AppEui,\""  + String(APPEUI) + "\"").c_str()); delay(200);
        sendAT(("AT+KEY=AppKey,\"" + String(APPKEY) + "\"").c_str()); delay(200);
        esp_task_wdt_reset();

        Serial.println("[LoRa-E5] AT+JOIN... (jusqu'à 15s)");
        resp = sendAT("AT+JOIN", 15000);

        if (resp.indexOf("Network joined") < 0) {
            Serial.println("[LoRa-E5] JOIN ECHOUE");
            Serial.println("  → Vérifier clés TTN, gateway à portée");
            goToSleep();
        }

        Serial.println("[LoRa-E5] JOIN OK !");
        rtcJoined = true;

    } else {

        Serial.println("[LoRa-E5] === Vérification session existante ===");
        esp_task_wdt_reset();

        resp = sendAT("AT+JOIN", 10000);

        if (resp.indexOf("Network joined") < 0 &&
            resp.indexOf("Already joined") < 0) {
            Serial.println("[LoRa-E5] Session perdue — reconfiguration au prochain réveil");
            rtcJoined = false;
            goToSleep();
        }

        Serial.println("[LoRa-E5] Session active OK");
        esp_task_wdt_reset();
    }

    // ── Envoi uplink ──────────────────────────────────────────
    Serial.printf("[LoRa-E5] Envoi payload : %s\n", payload);

    String msgCmd = String("AT+MSGHEX=\"") + payload + "\"";
    resp = sendAT(msgCmd.c_str(), 10000);

    if (resp.indexOf("Done") >= 0) {
        Serial.println("[LoRa-E5] ✓ Envoi OK !");
    } else if (resp.indexOf("ERROR") >= 0) {
        Serial.printf("[LoRa-E5] ✗ Erreur : %s\n", resp.c_str());
    } else {
        Serial.println("[LoRa-E5] Timeout — probablement envoyé sans downlink");
    }

    // ── Deep sleep ────────────────────────────────────────────
    goToSleep();
}

// ================================================================
//  LOOP — jamais atteint
// ================================================================
void loop() {
    goToSleep();
}