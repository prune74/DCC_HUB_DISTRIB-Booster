// GlobalInject.cpp — repli global de dernier recours (niveau 3)
#include <Arduino.h>
#include "GlobalInject.h"
#include "Pins.h"
#include "Config.h"

static bool active = false;

void globalInjectInit() {
    pinMode(HUB_DAC_INJECT_PIN, OUTPUT);
    dacWrite(HUB_DAC_INJECT_PIN, 0);
}

void globalInject(bool on) {
    if (on && !active) {
        dacWrite(HUB_DAC_INJECT_PIN, DAC_INJECT_VALUE);
        active = true;
        Serial.printf("[global] injection LaBox ON (hub %c)\n", 'A' + HUB_INSTANCE);
    } else if (!on && active) {
        dacWrite(HUB_DAC_INJECT_PIN, 0);
        active = false;
        Serial.printf("[global] injection LaBox OFF (hub %c)\n", 'A' + HUB_INSTANCE);
    }
}

bool globalInjectActive() { return active; }