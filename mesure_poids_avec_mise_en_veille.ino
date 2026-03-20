#include <Arduino.h>
#include <HX711.h>
#include "driver/gpio.h"

// ── Pins HX711 ────────────────────────────────────────────────
#define DT_PIN 32
#define SCK_PIN 33

// ── Calibration HX711 ─────────────────────────────────────────
#define FACTEUR_CALIBRATION 30006.67
#define OFFSET_TARE 140891

// ── Paramètres de lecture ─────────────────────────────────────
#define HX711_SAMPLES 8

HX711 balance;

// ================================================================
// Mise en veille du HX711
// SCK HIGH > 60 µs pour passer en power-down
// ================================================================
void powerDownHX711() {
    pinMode(SCK_PIN, OUTPUT);
    digitalWrite(SCK_PIN, HIGH);
    delayMicroseconds(80);

    gpio_config_t cfg = {};
    cfg.pin_bit_mask    = (1ULL << SCK_PIN);
    cfg.mode            = GPIO_MODE_INPUT;
    cfg.pull_up_en      = GPIO_PULLUP_DISABLE;
    cfg.pull_down_en    = GPIO_PULLDOWN_DISABLE;
    cfg.intr_type       = GPIO_INTR_DISABLE;
    gpio_config(&cfg);
}

// ================================================================
// Lecture du poids
// ================================================================
float lirePoidsKg() {
    if (!balance.is_ready()) {
        Serial.println("[HX711] Erreur : module non pret");
        return -1.0f;
    }

    float poids = balance.get_units(HX711_SAMPLES);

    if (poids < 0) {
        poids = 0;
    }

    return poids;
}

// ================================================================
// SETUP
// ================================================================
void setup() {
    Serial.begin(115200);
    delay(1000);

    Serial.println("=== Test capteur poids ruche / HX711 ===");

    balance.begin(DT_PIN, SCK_PIN);
    balance.set_offset(OFFSET_TARE);
    balance.set_scale(FACTEUR_CALIBRATION);

    Serial.println("[HX711] Initialisation terminee");
}

// ================================================================
// LOOP
// ================================================================
void loop() {
    float poidsKg = lirePoidsKg();

    if (poidsKg >= 0.0f) {
        Serial.print("[HX711] Poids : ");
        Serial.print(poidsKg, 2);
        Serial.println(" kg");
    }

    delay(2000);
}
