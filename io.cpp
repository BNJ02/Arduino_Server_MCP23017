#include "io.h"

bool MCP23017::begin(uint8_t addr, TwoWire *wire) {
    if ((addr >= 0x20) && (addr <= 0x27)) {
        _i2caddr = addr;
    } else if (addr <= 0x07) {
        _i2caddr = 0x20 + addr;
    } else {
        _i2caddr = 0x27;
    }
    wire->begin(); // Initialiser la communication I2C

    return true;
}

void MCP23017::setSpeed(void) {
    Wire.setClock(400000);  // 400 kHz pour une communication rapide
}

bool MCP23017::pinMode(uint8_t port, uint8_t pin, uint8_t mode) {
    if (pin > 7 || port > 1) return false; // MCP23017 gère 8 broches max sur 2 ports

    uint8_t iodir = read8(MCP23017_IODIRA + port); // Lire le registre IODIR
    if (mode == INPUT) {
        iodir |= (1 << pin); // Mettre la broche en entrée
    } else {
        iodir &= ~(1 << pin); // Mettre la broche en sortie
    }
    return write8(MCP23017_IODIRA + port, iodir); // Écrire dans le registre IODIR
}

bool MCP23017::pullUp(uint8_t port, uint8_t pin, uint8_t mode) {
    if (pin > 7 || port > 1) return false; // MCP23017 gère 8 broches max sur 2 ports

    uint8_t gppu = read8(MCP23017_GPPUA + port); // Lire le registre GPPU
    if (mode == HIGH) {
        gppu |= (1 << pin); // Activer le pull-up sur la broche
    } else {
        gppu &= ~(1 << pin); // Désactiver le pull-up sur la broche
    }
    return write8(MCP23017_GPPUA + port, gppu); // Écrire dans le registre GPPU
}

uint8_t MCP23017::digitalRead(uint8_t port, uint8_t pin) {
    if (pin > 7 || port > 1) return 0; // MCP23017 gère 8 broches max sur 2 ports

    uint8_t gpio = read8(MCP23017_GPIOA + port); // Lire l'état des broches
    return (gpio >> pin) & 0x1; // Retourner l'état de la broche
}

void MCP23017::digitalWrite(uint8_t port, uint8_t pin, uint8_t state) {
    if (pin > 7 || port > 1) return; // MCP23017 gère 8 broches max sur 2 ports

    uint8_t gpio = read8(MCP23017_OLATA + port); // Lire l'état actuel du registre OLAT
    if (state == HIGH) {
        gpio |= (1 << pin); // Mettre la broche à HIGH
    } else {
        gpio &= ~(1 << pin); // Mettre la broche à LOW
    }
    write8(MCP23017_OLATA + port, gpio); // Écrire dans le registre OLAT
}

uint8_t MCP23017::read8(uint8_t addr) {
    Wire.beginTransmission(_i2caddr);
    Wire.write(addr); // Spécifier l'adresse du registre
    Wire.endTransmission();

    Wire.requestFrom(_i2caddr, (uint8_t)1); // Demander un octet du registre
    if (Wire.available()) {
        return Wire.read(); // Retourner la valeur lue
    }

    return 0; // Retourner 0 si aucune donnée disponible
}

bool MCP23017::write8(uint8_t addr, uint8_t data) {
    Wire.beginTransmission(_i2caddr);
    Wire.write(addr); // Spécifier l'adresse du registre
    Wire.write(data); // Écrire la valeur
    return Wire.endTransmission() == 0; // Retourner true si la transmission est réussie
}

// Activer les interruptions sur une broche spécifique
bool MCP23017::enableInterrupt(uint8_t port, uint8_t pin, bool enable, uint8_t mode) {
    if (pin > 7 || port > 1) return false; // MCP23017 gère 8 broches max sur 2 ports

    uint8_t gpinten = read8(MCP23017_GPINTENA + port); // Lire l'état actuel du registre GPINTEN
    uint8_t intcon = read8(MCP23017_INTCONA + port);   // Lire l'état actuel du registre INTCON
    uint8_t defval = read8(MCP23017_DEFVALA + port);   // Lire l'état actuel du registre DEFVAL

    if (enable) {
        gpinten |= (1 << pin);   // Activer l'interruption sur la broche p
        
        if (mode == CHANGE) {
            intcon &= ~(1 << pin); // Interruption sur changement d'état
        } else {
            intcon |= (1 << pin);  // Interruption sur comparaison avec DEFVAL
            if (mode == FALLING) {
                defval |= (1 << pin);  // Interruption sur descente (HIGH -> LOW)
            } else if (mode == RISING) {
                defval &= ~(1 << pin); // Interruption sur montée (LOW -> HIGH)
            }
        }
    } else {
        gpinten &= ~(1 << pin); // Désactiver l'interruption sur la broche p
    }

    // Écriture des modifications dans les registres
    bool success = write8(MCP23017_GPINTENA + port, gpinten);
    success &= write8(MCP23017_INTCONA + port, intcon);
    success &= write8(MCP23017_DEFVALA + port, defval);
    return success;
}

// Retourne le numéro de la broche qui a déclenché l'interruption sur le port donné
int8_t MCP23017::getInterruptPin(uint8_t port) {
    if (port > 1) return -1; // MCP23017 gère uniquement 2 ports (A et B)

    uint8_t intf = read8(MCP23017_INTFA + port); // Lire le registre INTF pour savoir quelle broche a déclenché
    for (uint8_t pin = 0; pin < 8; pin++) {
        if (intf & (1 << pin)) {
            return pin; // Retourner le premier pin ayant déclenché l'interruption
        }
    }
    return -1; // Retourne -1 si aucune broche n'a déclenché
}

// Retourne true si l'interruption était un front montant (rising edge), false si c'était un front descendant (falling edge)
bool MCP23017::isRisingEdge(uint8_t port, uint8_t pin) {
    if (pin > 7 || port > 1) return false;

    uint8_t intcap = read8(MCP23017_INTCAPA + port); // Lire l'état de la broche au moment de l'interruption
    return (intcap & (1 << pin)); // Retourne true si INTCAP est bas (0), indiquant un front montant
}
