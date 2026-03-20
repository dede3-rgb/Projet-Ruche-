#include <Arduino.h>
#include <HX711.h>

#define DT_PIN 32
#define SCK_PIN 33

#define FACTEUR_CALIBRATION 30006.67
#define OFFSET_TARE 140891
#define HX711_SAMPLES 8

HX711 balance;

void setup() {
    Serial.begin(115200);
    delay(1000);

    balance.begin(DT_PIN, SCK_PIN);
    balance.set_offset(OFFSET_TARE);
    balance.set_scale(FACTEUR_CALIBRATION);

    Serial.println("HX711 pret");
}

void loop() {
    if (balance.is_ready()) {
        float poids = balance.get_units(HX711_SAMPLES);
        if (poids < 0) poids = 0;

        Serial.print("Poids : ");
        Serial.print(poids, 2);
        Serial.println(" kg");
    } else {
        Serial.println("HX711 non pret");
    }

    delay(2000);
}
