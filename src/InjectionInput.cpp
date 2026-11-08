// InjectionInput.cpp — 8 entrées directes (HUB_INJ_PINS), sans MCP23017.
// Les lignes sont TOUJOURS pilotées par le DAC du convertisseur (0 V ou
// ~3,04 V via résistance série) : simple digitalRead, aucun pull-up, aucune
// interruption I2C. L'anti-rebond (INJECT_CONFIRM_MS = 20 ms) est assuré par
// DistrictManager : le polling à 2 ms est largement suffisant.
#include "InjectionInput.h"
#include "Pins.h"
#include "Config.h"

static const gpio_num_t INJ_PIN[HUB_NB_DISTRICTS] = HUB_INJ_PINS;
static uint8_t level = 0;      // niveau TOR courant (bit n)

void injInit() {
    for (uint8_t i = 0; i < HUB_NB_DISTRICTS; i++) {
        pinMode(INJ_PIN[i], INPUT);   // 34/35/36/39 : input only, sans pull-up,
                                      // lignes pilotées par les convertisseurs
    }
    level = 0;
    for (uint8_t i = 0; i < HUB_NB_DISTRICTS; i++) level |= digitalRead(INJ_PIN[i]) << i;
}

void injPoll() {
    uint8_t lvl = 0;
    for (uint8_t i = 0; i < HUB_NB_DISTRICTS; i++) {
        lvl |= (uint8_t)digitalRead(INJ_PIN[i]) << i;
    }
    level = lvl;
}

bool injActive(uint8_t n) {
    return (n < HUB_NB_DISTRICTS) && ((level >> n) & 1);
}