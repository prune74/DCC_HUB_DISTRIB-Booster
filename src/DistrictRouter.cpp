// DistrictRouter.cpp — GPIO Matrix : le signal d'entrée du GPIO n vaut 0x100 + n
#include "DistrictRouter.h"
#include "Pins.h"
#include "Config.h"
#include "driver/gpio.h"

static const gpio_num_t DCC_OUT[HUB_NB_DISTRICTS] = HUB_DCC_OUT_PINS;
static bool enState[HUB_NB_DISTRICTS];

void routerInit() {
    pinMode(HUB_DCC_IN_PIN, INPUT);
    for (uint8_t i = 0; i < HUB_NB_DISTRICTS; i++) {
        gpio_reset_pin(DCC_OUT[i]);
        gpio_set_direction(DCC_OUT[i], GPIO_MODE_OUTPUT);
        enState[i] = true;
    }
    routerSetAll(true);    // démarrage : tous les districts actifs
}

void routerSetDistrict(uint8_t n, bool on) {
    if (n >= HUB_NB_DISTRICTS) return;
    if (on) {
        // reconfigure la broche en sortie simple, puis route le signal d'entrée
        gpio_reset_pin(DCC_OUT[n]);
        gpio_set_direction(DCC_OUT[n], GPIO_MODE_OUTPUT);
        gpio_matrix_out(DCC_OUT[n], 0x100 + HUB_DCC_IN_PIN, false, false);
    } else {
        pinMode(DCC_OUT[n], OUTPUT);   // GPIO simple : la matrice est ignorée
        digitalWrite(DCC_OUT[n], LOW);
    }
    enState[n] = on;
}

bool routerGetDistrict(uint8_t n) {
    return (n < HUB_NB_DISTRICTS) && enState[n];
}

void routerSetAll(bool on) {
    for (uint8_t i = 0; i < HUB_NB_DISTRICTS; i++) routerSetDistrict(i, on);
}