// Pins.h — brochage du hub de distribution (8 districts)
#pragma once

#define HUB_DCC_IN_PIN        27   // XOR SN74LVC1G86 <- LaBox EXT
#define HUB_DAC_INJECT_PIN    25   // -> broche intensité EXT (GPIO35 hub A / GPIO39 hub B)
#define HUB_I2C_SDA           21
#define HUB_I2C_SCL           22
#define HUB_MCP_INT_PIN       34   // INT du MCP23017 (open-drain, pull-up 10 kΩ)
#define HUB_CAN_TX            GPIO_NUM_5   // TWAI interne via CanUniversal
#define HUB_CAN_RX            GPIO_NUM_4   // (convention DCC_CAN_RMT-Booster : TX=5, RX=4)
#define HUB_WS2812_PIN        26   // 1 + HUB_NB_DISTRICTS LED

#define MCP_ADDR_INJ          0x20  // MCP23017 : 8 entrées injections (GPA0-7)

// DCC out : une broche par district (copie GPIO Matrix, coupable individuellement)
#define HUB_DCC_OUT_PINS      { 13, 14, 15, 18, 19, 16, 17, 32 }