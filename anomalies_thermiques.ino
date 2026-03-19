#include <DHT.h>

// --- Configuration Matérielle (Validée sur ton schéma) ---
#define PIN_DHT 25      // Pin du capteur DHT22 (Signal)
#define DHTTYPE DHT22   // Type de capteur utilisé

// --- Paramètres des Alertes (Tickets Jira BEEC-56) ---
const float TEMP_MAX = 38.0; // Seuil d'alerte haute (Surchauffe)
const float TEMP_MIN = 10.0; // Seuil d'alerte basse (Froid critique)

DHT dht(PIN_DHT, DHTTYPE);

void setup() {
  // Moniteur série pour le PC
  Serial.begin(115200);
  
  // Communication avec le module LoRa-E5 (Pins 16 RX / 17 TX)
  Serial2.begin(9600, SERIAL_8N1, 16, 17); 
  
  dht.begin();
  
  Serial.println("--- BEE CONNECTED : MONITORING THERMIQUE ---");
  Serial.print("Capteur DHT22 sur Pin : ");
  Serial.println(PIN_DHT);
}

void loop() {
  // Lecture de la température actuelle
  float tempActuelle = dht.readTemperature();

  // Vérification si le capteur répond
  if (isnan(tempActuelle)) {
    Serial.println("Erreur : Impossible de lire le capteur DHT22 sur la Pin 25 !");
  } else {
    Serial.print("Température ruche : ");
    Serial.print(tempActuelle);
    Serial.println(" °C");

    // Algorithme de test des seuils (Ticket BEEC-57)
    if (tempActuelle > TEMP_MAX) {
      Serial.println("!!! ALERTE : SURCHAUFFE DETECTEE !!!");
      envoyerAlerteLoRa("A2"); // Code hexa pour température haute
    } 
    else if (tempActuelle < TEMP_MIN) {
      Serial.println("!!! ALERTE : FROID CRITIQUE !!!");
      envoyerAlerteLoRa("A3"); // Code hexa pour température basse
    }
  }

  // Intervalle de mesure (10 secondes pour faciliter ta démo de Revue 2)
  delay(10000); 
}

// Fonction d'envoi réel vers TTN / BEEP (Ticket BEEC-55)
void envoyerAlerteLoRa(String codeHexa) {
  // Envoi de la commande AT au module Lora-E5
  Serial2.print("AT+MSGHEX=\"" + codeHexa + "\"\r\n");
  
  Serial.println(">> Commande LoRa envoyée : AT+MSGHEX=\"" + codeHexa + "\"");
}
