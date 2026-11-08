// StatusLed.cpp — codes couleurs cohérents avec DCC_CAN_RMT-Booster
#include "StatusLed.h"
#include "Config.h"
#include "Pins.h"
#include "DistrictManager.h"
#include "GlobalInject.h"
#include "InjectionInput.h"
#include <Ws2812Universal.h>

// --- Configuration matérielle via Provider (pattern CanUniversal) ---
class HubLedConfig : public Ws2812ConfigProvider {
public:
    uint8_t busCount() const override { return 1; }
    const Ws2812BusConfig &busConfig(uint8_t) const override {
        static const Ws2812BusConfig cfg(
            true,                                  // enabled
            HUB_WS2812_PIN,                        // GPIO 26
            1 + HUB_NB_DISTRICTS,                  // 9 LED : générale + 8 districts
            HUB_WS2812_RMT_CHANNEL,                // cohabitation RMT
            WS2812_BRIGHTNESS);
        return cfg;
    }
};

static Ws2812Bus &strip() { return Ws2812Bus::bus(0); }

void ledInit() {
    HubLedConfig config;
    Ws2812Init::begin(config);          // tolérant : bus invalide plutôt que crash
    strip().clear();
    strip().show();                     // LED éteintes au boot
}

static uint32_t color(uint8_t r, uint8_t g, uint8_t b) { return ((uint32_t)r << 16) | ((uint32_t)g << 8) | b; }

void ledTick() {
    uint32_t now = millis();
    bool blink = (now / 500) & 1;

    // LED 0 — général : rouge clignotant si injection globale, battement de coeur sinon
    if (globalInjectActive())          strip().setPixel(0, blink ? color(80,0,0) : color(0,0,0));
    else if ((now / 1500) % 2 == 0)    strip().setPixel(0, color(0,20,0));
    else                               strip().setPixel(0, color(0,4,0));

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
        strip().setPixel(1 + i, c);
    }
    strip().show();                     // non bloquant (RMT), retourne false si trame en cours
}