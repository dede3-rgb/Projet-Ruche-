# 🐝 Projet Ruche Connectée (ESP32 + LoRa)

## 📌 Description

Ce projet permet de surveiller une ruche connectée grâce à un **ESP32** et une communication **LoRa (TTN)**.
Il collecte plusieurs données environnementales et les envoie périodiquement vers le réseau.

## 📌 Notifications Ubidots

Les alertes sont finalement recues ici : 

mail : ruchey22026@outlook.fr
mot de passe : Ruche.Y2@2026

## ⚙️ Fonctionnalités

* 🌡️ Température & humidité (x2 capteurs DHT22)
* 🌡️ Température interne (DS18B20 x2)
* ⚖️ Poids de la ruche (HX711 + cellule de charge)
* 💡 Luminosité (BH1750)
* 🔋 Niveau de batterie
* 📡 Transmission LoRa (OTAA via The Things Network)
* 💤 Deep Sleep configurable (économie d’énergie)
* 🔔 Buzzer au démarrage
* ⬇️ Downlink pour modifier le temps de sleep


## 🔌 Matériel utilisé

* ESP32 (Low Power DevKit recommandé)
* Module LoRa UART
* 2x DHT22 / AM2302
* 1-2x DS18B20
* HX711 + cellule de charge
* BH1750 (I2C)
* Buzzer
* Batterie Li-ion


## 🔧 Configuration des pins

| Fonction     | GPIO |
| ------------ | ---- |
| LoRa RX      | 17   |
| LoRa TX      | 16   |
| MOSFET Power | 12   |
| Buzzer       | 4    |
| DHT22 #1     | 23   |
| DHT22 #2     | 25   |
| HX711 DT     | 32   |
| HX711 SCK    | 33   |
| DS18B20      | 27   |
| Batterie ADC | 35   |
| I2C SDA      | 21   |
| I2C SCL      | 22   |

## 📡 Configuration LoRa (TTN)

Remplacer les clés dans le code :

```cpp
#define DEVEUI "XXXXXXXXXXXXXXX"
#define APPEUI "0000000000000000"
#define APPKEY "XXXXXXXXXXXXXXXXXXXXXXXXXXXX"


* Mode : **OTAA**
* Région : **EU868**
* Classe : **A**

## 📦 Format du payload

Les données sont envoyées en **hexadécimal** :

| Donnée     | Format        |
| ---------- | ------------- |
| Temp 1     | int16 (x10)   |
| Hum 1      | int16 (x10)   |
| Temp 2     | int16 (x10)   |
| Hum 2      | int16 (x10)   |
| DS18B20 #1 | int16 (x10)   |
| DS18B20 #2 | int16 (x10)   |
| Luminosité | uint16        |
| Poids      | uint16 (x10)  |
| Batterie   | uint16 (x100) |


## 💤 Gestion de l’énergie

* Deep sleep activé après chaque envoi
* Durée par défaut : **10 minutes**
* Modifiable via downlink

### ⬇️ Downlink

* 1 octet = minutes de sleep (1 à 255)

Exemple :

0A → 10 minutes
1E → 30 minutes

## 🔋 Batterie

* Lecture via ADC GPIO35
* Calibration :

#define BATTERY_FACTOR 1.36f

* Conversion en % :
* 3.2V → 0%
* 4.2V → 100%

## 🚀 Fonctionnement

1. Boot ESP32
2. Bip du buzzer
3. Initialisation capteurs
4. Connexion LoRa (JOIN OTAA)
5. Lecture des capteurs
6. Envoi du payload
7. Réception éventuelle d’un downlink
8. Passage en deep sleep


## 🛠️ Développement

Projet développé avec :

* Arduino IDE ou Visual Studio Code (PlatformIO recommandé)


## 📜 Licence

Projet libre d’utilisation pour projets personnels et éducatifs.


## 👨‍💻 Auteur

Projet réalisé par 

Dede, Ajinthini, Nassim, Hazem 

EI4-FISA 2026

Polytech Sorbonne
