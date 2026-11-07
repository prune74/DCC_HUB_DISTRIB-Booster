// HubCanConfig.h — configuration matérielle CAN fournie à CanUniversal (Provider)
#pragma once

#include <CanUniversal.h>
#include "Config.h"
#include "Pins.h"

// Bus CAN unique : TWAI interne (cs = GPIO_NUM_NC).
// enabled, speed, tx, rx, cs, int, sck, mosi, miso, quartz, tolérance, loopback
struct HubCanConfig : public CanConfigProvider
{
    CanBusConfig buses[1] = {
        CanBusConfig(true, CAN_BITRATE,
                     HUB_CAN_TX, HUB_CAN_RX, // TWAI interne
                     GPIO_NUM_NC, GPIO_NUM_NC,
                     GPIO_NUM_NC, GPIO_NUM_NC, GPIO_NUM_NC,
                     0, 0,
                     false)};

    uint8_t busCount() const override { return 1; }
    const CanBusConfig &bus(uint8_t index) const override { return buses[0]; }
};