#pragma once

#include <Arduino.h>
#include <Wire.h>

#define MCP23017_ADDRESS 0x20

#define MCP23017_IODIRA 0x00
#define MCP23017_IODIRB 0x01
#define MCP23017_IPOLA 0x02
#define MCP23017_IPOLB 0x03
#define MCP23017_GPINTENA 0x04
#define MCP23017_GPINTENB 0x05
#define MCP23017_DEFVALA 0x06
#define MCP23017_DEFVALB 0x07
#define MCP23017_INTCONA 0x08
#define MCP23017_INTCONB 0x09
#define MCP23017_IOCONA 0x0A
#define MCP23017_IOCONB 0x0B
#define MCP23017_GPPUA 0x0C
#define MCP23017_GPPUB 0x0D
#define MCP23017_INTFA 0x0E
#define MCP23017_INTFB 0x0F
#define MCP23017_INTCAPA 0x10
#define MCP23017_INTCAPB 0x11
#define MCP23017_GPIOA 0x12
#define MCP23017_GPIOB 0x13
#define MCP23017_OLATA 0x14
#define MCP23017_OLATB 0x15

#define PORTA 0
#define PORTB 1

class MCP23017 {
    public:
        bool begin(uint8_t addr = 0x20, TwoWire *wire = &Wire);
        void setSpeed(void);

        // Basics
        bool pinMode(uint8_t port, uint8_t pin, uint8_t mode);
        bool pullUp(uint8_t port, uint8_t pin, uint8_t mode);
        uint8_t digitalRead(uint8_t port, uint8_t pin);
        void digitalWrite(uint8_t port, uint8_t pin, uint8_t state);

        // Interrupts
        bool enableInterrupt(uint8_t port, uint8_t pin, bool enable, uint8_t mode);
        int8_t getInterruptPin(uint8_t port);
        uint8_t getInterruptPinValue(uint8_t port);
        
        // Low-level access
        uint8_t read8(uint8_t addr);
        bool write8(uint8_t addr, uint8_t data);

    private:
        uint8_t _i2caddr;
};