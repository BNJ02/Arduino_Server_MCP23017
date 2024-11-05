#include <SPI.h>
#include <Ethernet.h>
#include "io.h"

// Arduino Mega 2560
// Pin #1 of the expander to GPIO21 (i2c clock)
// Pin #2 of the expander to GPIO20 (i2c data)
// Pins #3, 4 and 5 of the expander to ground (address selection)
// Pin #6 and 18 of the expander to 5V (power and reset disable)
// Pin #9 of the expander to ground (common ground)

// Commenter cette ligne pour désactiver le mode débogage (affichage des messages de débogage sur le port série 115200)
#define DEBUG

#ifdef DEBUG
    #define DEBUG_PRINTLN(x)  Serial.println(x)
    #define DEBUG_PRINT(x)    Serial.print(x)
#else
    #define DEBUG_PRINTLN(x)  // Rien
    #define DEBUG_PRINT(x)    // Rien
#endif

// Configuration de l'adresse MAC et de l'IP de l'Arduino
const byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
const IPAddress ip(192, 168, 1, 177);

// Initialisation du serveur Ethernet sur le port 80 et du client
EthernetServer server(80);
EthernetClient client;

// Initialisation des variables pour la détection d'interruption
const int interruptPinPortA = 18; // Configuration de la broche de détection d'interruption pour le port A
const int interruptPinPortB = 19; // Configuration de la broche de détection d'interruption pour le port B
const int ledPin = A7;
const int ledFlashPin = A15;

const unsigned long debounceDelay = 50000; // Délai de détection anti-rebond en microsecondes

volatile unsigned long interruptFlagPortA = 0; // Flag pour indiquer un changement contenant la date de l'interruption
volatile unsigned long interruptFlagPortB = 0; // Flag pour indiquer un changement contenant la date de l'interruption

MCP23017 mcp; // Déclaration de mon composant MCP23017

// Déclaration de la fonction d'interruption pour le port A
void handleInterruptPortA() {
    static unsigned long lastChangeTime = 0;
    unsigned long currentTime = micros();

    if (currentTime - lastChangeTime > debounceDelay) {
        interruptFlagPortA = currentTime;  // Définit le flag pour indiquer un changement avec la date de l'interruption
        lastChangeTime = currentTime;
    }
}

// Déclaration de la fonction d'interruption pour le port B
void handleInterruptPortB() {
    static unsigned long lastChangeTime = 0;
    unsigned long currentTime = micros();

    if (currentTime - lastChangeTime > debounceDelay) {
        interruptFlagPortB = currentTime;  // Définit le flag pour indiquer un changement avec la date de l'interruption
        lastChangeTime = currentTime;
    }
}

void setup() {
    // Initialisation de l'Ethernet
    Ethernet.begin(mac, ip);

    // Démarrage du serveur
    server.begin();
#ifdef DEBUG
    Serial.begin(115200);
#endif
    DEBUG_PRINTLN("Serveur pret");

    // Initialisation du composant MCP23017
    mcp.begin();
    mcp.setSpeed(); // Utiliser la vitesse maximale de l'I2C

    // Configuration des broches du MCP23017 en entrée sur les ports A et B
    for (int i = 0; i < 8; i++) mcp.pinMode(PORTA, i, INPUT);
    for (int i = 0; i < 8; i++) mcp.pinMode(PORTB, i, INPUT);

    // Activation des résistances de pull-up sur les broches du MCP23017 pour les ports A et B
    for (int i = 0; i < 8; i++) mcp.pullUp(PORTA, i, HIGH);
    for (int i = 0; i < 8; i++) mcp.pullUp(PORTB, i, HIGH);

    // Configuration des interruptions sur le MCP23017 pour les ports A et B
    for (int i = 0; i < 8; i++) mcp.enableInterrupt(PORTA, i, true, CHANGE);
    for (int i = 0; i < 8; i++) mcp.enableInterrupt(PORTB, i, true, CHANGE);
    mcp.read8(MCP23017_INTCAPA); // Lecture du registre d'interruption pour réinitialiser l'interruption sur le port A
    mcp.read8(MCP23017_INTCAPB); // Port B

    // Configuration des broches d'interruption du MCP en entrée pour les ports A et B
    pinMode(interruptPinPortA, INPUT_PULLUP);
    pinMode(interruptPinPortB, INPUT_PULLUP);

    // Configuration de l'interruption sur le GPIO 10
    attachInterrupt(digitalPinToInterrupt(interruptPinPortA), handleInterruptPortA, FALLING);
    attachInterrupt(digitalPinToInterrupt(interruptPinPortB), handleInterruptPortB, FALLING);

    // Configuration de la LED sur le GPIO analog A10
    pinMode(ledPin, OUTPUT);
    pinMode(ledFlashPin, OUTPUT);
}

// Fonction pour transférer un message au client
void transfer_message (const String msg) {
    unsigned long startTime = 0;
    static unsigned long ping = 0;

    startTime = micros();               // Stocke le temps actuel en microsecondes
    String message = msg + "\t\ttime : " + String(startTime) + " us\t\tping : " + String(ping) + " us";

    // Envoi de la réponse au client
    server.print(message);

    // Lecture de la requête du client (acknowledge)
    String request = "";
    if (client.available()) {
        ping = micros() - startTime;

        while (client.available()) {
            char c = client.read();
            request += c;
        }
    }
    // DEBUG_PRINTLN(request);
}

void loop() {
    client = server.available();
    if (client) {
        DEBUG_PRINTLN("Nouveau client");

        unsigned int i = 0;
        unsigned long previousMillis = 0;
        unsigned long previousMillisLed = 0;
        const unsigned int interval = 1000;  // Intervalle de 1 seconde

        while (client.connected()) {
            unsigned long currentMillis = millis();
            static bool tempoLed = false;

            // Envoi du message "Still alive" tous les intervalles
            if (currentMillis - previousMillis >= interval) {
                previousMillis = currentMillis;
                ++i;
                String message = "Still ALIVE " + String(i) + "\t\t";
                transfer_message(message);
                DEBUG_PRINTLN(message);

                mcp.read8(MCP23017_INTCAPA); // Lecture du registre d'interruption pour réinitialiser l'interruption sur le port A
                mcp.read8(MCP23017_INTCAPB); // Port B
            }

            // LED allumée pendant 1 seconde si une interruption est détectée
            if (tempoLed && currentMillis - previousMillisLed >= interval) {
                digitalWrite(ledFlashPin, LOW);
                tempoLed = false;
            }

            // Section critique pour vérifier et réinitialiser les interruptions
            noInterrupts();  // Désactiver les interruptions
            unsigned long localFlagPortA = interruptFlagPortA;
            unsigned long localFlagPortB = interruptFlagPortB;
            interruptFlagPortA = 0;  // Réinitialiser les drapeaux dans la section critique
            interruptFlagPortB = 0;
            interrupts();  // Réactiver les interruptions

            // Si interruption détectée sur le port A
            if (localFlagPortA) {
                int8_t GP_pin_detected = mcp.getInterruptPin(PORTA);
                String interruptMessage = "GPIO" + String(GP_pin_detected) + " detected, delay : " + String(micros() - localFlagPortA) + " us";
                transfer_message(interruptMessage);
                DEBUG_PRINTLN(interruptMessage);

                digitalWrite(ledPin, mcp.digitalRead(PORTA, GP_pin_detected));
                tempoLed = true;
                previousMillisLed = currentMillis;
                digitalWrite(ledFlashPin, HIGH);
            }

            // Si interruption détectée sur le port B
            if (localFlagPortB) {
                int8_t GP_pin_detected = mcp.getInterruptPin(PORTB);
                String interruptMessage = "GPIO" + String(GP_pin_detected + 8) + " detected, delay : " + String(micros() - localFlagPortB) + " us";
                transfer_message(interruptMessage);
                DEBUG_PRINTLN(interruptMessage);

                digitalWrite(ledPin, mcp.digitalRead(PORTB, GP_pin_detected));
                tempoLed = true;
                previousMillisLed = currentMillis;
                digitalWrite(ledFlashPin, HIGH);
            }
        }

        // Déconnexion du client
        client.stop();
        DEBUG_PRINTLN("Client deconnecte");
    }
}
