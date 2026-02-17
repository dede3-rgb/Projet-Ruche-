#include <HX711.h>

#define DT_PIN  32
#define SCK_PIN 33

// ⚠️ CE FACTEUR EST À AJUSTER APRÈS CALIBRATION
#define FACTEUR_CALIBRATION 20.08

HX711 balance;

void setup() {
  Serial.begin(115200);
  balance.begin(DT_PIN, SCK_PIN);
  
  Serial.println("Initialisation...");
  delay(2000);
  
  // Tare automatique au démarrage
  balance.tare();
  balance.set_scale(FACTEUR_CALIBRATION);
  
  Serial.println("Balance prête ! Pose un objet...");
}

void loop() {
  if (balance.is_ready()) {
    
    // Valeur brute pour calibration
    long brut = balance.get_value(5);
    Serial.print("Brut : ");
    Serial.print(brut);
    
    // Poids en grammes
    float poids = balance.get_units(5);
    Serial.print("  |  Poids : ");
    Serial.print(poids, 1);
    Serial.println(" g");
    
  } else {
    Serial.println("HX711 non détecté !");
  }
  
  delay(500);
}