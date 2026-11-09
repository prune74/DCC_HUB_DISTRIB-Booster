// HubWs2812Config.h — configuration matérielle WS2812 fournie à Ws2812Universal (Provider)
#pragma once

#include <Ws2812Universal.h>
#include "Config.h"
#include "Pins.h"

// Bus WS2812 unique : 1 + HUB_NB_DISTRICTS LED (générale + 8 districts).
// enabled, pin, numPixels, rmtChannel, brightness
struct HubWs2812Config : public Ws2812ConfigProvider {
    uint8_t busCount() const override { return 1; }
    const Ws2812BusConfig& busConfig(uint8_t) const override {
        static const Ws2812BusConfig cfg(
            true,                           // enabled
            HUB_WS2812_PIN,                // GPIO 26
            1 + HUB_NB_DISTRICTS,          // 9 LED : générale + 8 districts
            HUB_WS2812_RMT_CHANNEL,        // cohabitation RMT
            WS2812_BRIGHTNESS);
        return cfg;
    }
};
