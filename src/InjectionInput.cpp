// InjectionInput.cpp — MCP23017 @ 0x20, port A (GPA0-7 = districts 1-8)
#include "InjectionInput.h"
#include "Pins.h"
#include "Config.h"
#include <Wire.h>

#define MCP_IODIRA 0x00
#define MCP_IPOLA  0x02
#define MCP_GPINTENA 0x04
#define MCP_DEFVALA  0x06
#define MCP_INTCONA  0x08
#define MCP_IOCON    0x0A
#define MCP_GPPUA  0x0C
#define MCP_INTCAPA 0x10
#define MCP_GPIOA  0x12

static volatile bool intPending = false;
static uint8_t level = 0;      // niveau TOR courant (bit n)

static void ARDUINO_ISR_ATTR onInt() { intPending = true; }

static uint8_t rd(uint8_t reg) {
    Wire.beginTransmission(MCP_ADDR_INJ);
    Wire.write(reg);
    Wire.endTransmission(false);
    Wire.requestFrom((int)MCP_ADDR_INJ, 1);
    return (uint8_t)Wire.read();
}

void injInit() {
    pinMode(HUB_MCP_INT_PIN, INPUT);
    attachInterrupt(digitalPinToInterrupt(HUB_MCP_INT_PIN), onInt, FALLING);
    auto wr = [](uint8_t reg, uint8_t val) {
        Wire.beginTransmission(MCP_ADDR_INJ);
        Wire.write(reg); Wire.write(val);
        Wire.endTransmission();
    };
    wr(MCP_IOCON, 0x44);        // INT open-drain
    wr(MCP_IODIRA, 0xFF);       // port A tout en entrée
    wr(MCP_IPOLA,  0x00);
    wr(MCP_GPPUA,  0xFF);       // pull-ups internes : LOW sans convertisseur
    wr(MCP_GPINTENA, 0xFF);     // interruption sur changement
    wr(MCP_INTCONA, 0x00);
    (void)rd(MCP_INTCAPA);      // acquitter l'INT de démarrage
    level = rd(MCP_GPIOA);
}

void injPoll() {
    if (!intPending) return;
    intPending = false;
    level = rd(MCP_GPIOA);
}

bool injActive(uint8_t n) {
    return (n < HUB_NB_DISTRICTS) && ((level >> n) & 1);
}