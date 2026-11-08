// StatusLed.cpp — codes couleurs cohérents avec DCC_CAN_RMT-Booster
#include "StatusLed.h"
#include "Config.h"
#include "Pins.h"
#include "DistrictManager.h"
#include "GlobalInject.h"
#include "InjectionInput.h"
#include <Adafruit_NeoPixel.h>

#define LED_COUNT (1 + HUB_NB_DISTRICTS)
static Adafruit_NeoPixel strip(LED_COUNT, HUB_WS2812_PIN, NEO_GRB + NEO_KHZ800);

void ledInit() {
    strip.begin();
    strip.setBrightness(WS2812_BRIGHTNESS);
    strip.show();
}

static uint32_t color(uint8_t r, uint8_t g, uint8_t b) { return strip.Color(r, g, b); }

void ledTick() {
    uint32_t now = millis();
    bool blink = (now / 500) & 1;

    // LED 0 — général : rouge clignotant si injection globale, battement de coeur sinon
    if (globalInjectActive())          strip.setPixelColor(0, blink ? color(80,0,0) : color(0,0,0));
    else if ((now / 1500) % 2 == 0)    strip.setPixelColor(0, color(0,20,0));
    else                               strip.setPixelColor(0, color(0,4,0));

    // LED 1..8 — districts
    for (uint8_t i = 0; i < HUB_NB_DISTRICTS; i++) {
        uint32_t c = color(0, 20, 0);                                    // ACTIF : vert discret
        switch (dmState(i)) {
        case DS_COUPE: case DS_ATTENTE_RETRY:
            c = color(80, 40, 0); break;                                // orange
        case DS_VERROUILLE:
            c = blink ? color(80, 0, 0) : color(0, 0, 0); break;        // rouge clignotant
        case DS_INJECTION_GLOBALE:
            c = color(80, 0, 80); break;                                // magenta
        case DS_COUPE_MANUEL:
            c = color(0, 0, 20); break;                                // bleu
        case DS_ACTIF:
            if (injActive(i)) c = color(80, 0, 0);                      // injection non confirmée : rouge
            break;
        }
        strip.setPixelColor(1 + i, c);
    }
    strip.show();
}