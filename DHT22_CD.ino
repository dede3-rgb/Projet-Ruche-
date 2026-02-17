#include <DHT.h>

#define PIN_DHT 23
#define TYPE_DHT DHT22

DHT capteur(PIN_DHT, TYPE_DHT);

void setup() {
  Serial.begin(115200);
  capteur.begin();
  Serial.println("Démarrage du capteur DHT22...");
}

void loop() {
  delay(2000);

  float temperature = capteur.readTemperature();
  float humidite = capteur.readHumidity();

  if (isnan(temperature) || isnan(humidite)) {
    Serial.println("Erreur de lecture du capteur !");
    return;
  }

  Serial.print("Température : ");
  Serial.print(temperature);
  Serial.println(" °C");

  Serial.print("Humidité    : ");
  Serial.print(humidite);
  Serial.println(" %");

  Serial.println("-------------------");
}