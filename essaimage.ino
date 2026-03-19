#include "HX711.h"

// Définition des pins pour le HX711 (Pins vérifiées : 32 et 33)
const int LOADCELL_DOUT_PIN = 32; 
const int LOADCELL_SCK_PIN = 33;

HX711 scale;

// Variables pour l'essaimage
float poidsActuel = 0.0;
float poidsPrecedent = 0.0;
const float SEUIL_ESSAIMAGE = 1.5; // Alerte si perte > 1.5 kg
unsigned long dernierCheck = 0;
const unsigned long INTERVALLE_CHECK = 600000; // 10 minutes en ms (60000 pour 1 min en démo)

void setup() {
  Serial.begin(115200);
  
  // Initialisation du port série pour le module LoRa (Pins 16/17)
  Serial2.begin(9600, SERIAL_8N1, 16, 17); 
  
  // Initialisation de la balance
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  
  // Utilisation de tes valeurs de calibration réelles
  scale.set_scale(30006.67f); 
  scale.set_offset(140891);
  
  // On fait une première lecture pour caler le poids de départ
  if (scale.is_ready()) {
    poidsPrecedent = scale.get_units(10);
    Serial.print("Poids initial de référence : ");
    Serial.print(poidsPrecedent);
    Serial.println(" kg");
  }
  
  Serial.println("Système BeeConnected prêt : Monitoring Essaimage (32/33)");
}

void loop() {
  // Lecture du poids en continu (moyenne sur 10 lectures pour la précision)
  if (scale.is_ready()) {
    poidsActuel = scale.get_units(10);
    // On affiche le poids toutes les 2 secondes pour voir que ça vit
    static unsigned long lastDisplay = 0;
    if (millis() - lastDisplay > 2000) {
      Serial.print("Poids actuel : ");
      Serial.print(poidsActuel);
      Serial.println(" kg");
      lastDisplay = millis();
    }
  }

  // Vérification de l'essaimage selon l'intervalle défini
  if (millis() - dernierCheck > INTERVALLE_CHECK) {
    
    // Calcul de la différence (Algorithme Ticket BEEC-52)
    float chuteDePoids = poidsPrecedent - poidsActuel;

    // Test du seuil (Ticket BEEC-54)
    if (chuteDePoids >= SEUIL_ESSAIMAGE) {
      Serial.println("!!! ALERTE ESSAIMAGE DÉTECTÉE !!!");
      envoyerAlerteLoRa(chuteDePoids); 
    }

    poidsPrecedent = poidsActuel; // Mise à jour pour le prochain cycle
    dernierCheck = millis();
  }
}

void envoyerAlerteLoRa(float perte) {
  // Envoi réel au module LoRa-E5 pour TTN/BEEP
  // On utilise un code hexadécimal simple (A1 pour essaimage)
  Serial2.print("AT+MSGHEX=\"A1\"\r\n"); 
  
  Serial.println("--- LOG TRANSMISSION ---");
  Serial.print("Commande AT envoyée : AT+MSGHEX=\"A1\" (Perte calculée : ");
  Serial.print(perte);
  Serial.println(" kg)");
  Serial.println("------------------------");
}
