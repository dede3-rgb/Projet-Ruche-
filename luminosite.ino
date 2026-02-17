#include <Wire.h>

// Adresses I2C typiques pour ce genre de capteur (à vérifier selon la puce)
#define I2C_ADDR 0x23 

void setup() {
  Serial.begin(115200);
  
  // Initialisation de l'I2C sur les pins du schéma (P21=SDA, P22=SCL)
  Wire.begin(21, 22); 
  
  Serial.println("--- Test I2C Luminosité (Schéma Prof) ---");
}

void loop() {
  uint16_t lux = 0;
  
  // Commande de lecture au capteur
  Wire.beginTransmission(I2C_ADDR);
  Wire.write(0x10); // Commande standard pour "Mesure Haute Résolution"
  Wire.endTransmission();
  
  delay(200); // Temps de mesure

  // Lecture des données (2 octets)
  Wire.requestFrom(I2C_ADDR, 2);
  if (Wire.available() >= 2) {
    lux = Wire.read();
    lux <<= 8;
    lux |= Wire.read();
    lux = lux / 1.2; // Facteur de conversion standard
  }

  // Affichage (Ticket BEEC-23)
  Serial.print("LUMINOSITE : ");
  Serial.print(lux);
  Serial.println(" Lux");

  delay(2000);
}
