#include <OneWire.h>

#define ONE_WIRE_BUS 32

OneWire ds(ONE_WIRE_BUS);
byte addr[8];

void setup() {
  Serial.begin(115200);

  // Cherche le capteur une seule fois
  if (!ds.search(addr)) {
    Serial.println("Erreur: DS18B20 non detecte");
    while (1);
  }

  ds.reset_search();
}

void loop() {

  byte data[9];

  // Lance conversion
  ds.reset();
  ds.select(addr);
  ds.write(0x44, 1);
  delay(750);

  // Lecture température
  ds.reset();
  ds.select(addr);
  ds.write(0xBE);

  for (int i = 0; i < 9; i++) {
    data[i] = ds.read();
  }

  int16_t raw = (data[1] << 8) | data[0];
  float temperatureC = raw / 16.0;

  Serial.print(temperatureC);
  Serial.println(" °C");

  delay(1000);
}
