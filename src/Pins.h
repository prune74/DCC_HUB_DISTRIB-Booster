// Pins.h — brochage du hub de distribution (8 districts)
// Toutes les broches sont de type gpio_num_t (GPIO_NUM_x), cohérent avec
// CanUniversal et les API driver/gpio. Les API Arduino (pinMode, digitalWrite,
// dacWrite, Wire.begin…) acceptent ces valeurs directement.
#pragma once

#include "driver/gpio.h"

// --- Entrée DCC (même convention que DCC_CAN_RMT-Booster)
#define HUB_DCC_IN_PIN        GPIO_NUM_27  // XOR SN74LVC1G86 <- LaBox EXT

// --- Sorties DCC : une broche par district (copie GPIO Matrix, coupable)
#define HUB_DCC_OUT_PINS     { GPIO_NUM_13, GPIO_NUM_14, GPIO_NUM_15, \
                               GPIO_NUM_18, GPIO_NUM_19, GPIO_NUM_16, \
                               GPIO_NUM_17, GPIO_NUM_32 }

// --- Injection globale (repli niveau 3 -> broche intensité EXT de LaBox)
#define HUB_DAC_INJECT_PIN    GPIO_NUM_25  // GPIO35 = EXT1 (hub A) / GPIO39 = EXT2 (hub B)

// --- I2C (MCP23017 : injections TOR des convertisseurs)
#define HUB_I2C_SDA           GPIO_NUM_21
#define HUB_I2C_SCL           GPIO_NUM_22
#define HUB_MCP_INT_PIN       GPIO_NUM_34  // INT du MCP23017 (open-drain, pull-up 10 kΩ)
#define MCP_ADDR_INJ          0x20         // 8 entrées injections (GPA0-7)

// --- CAN (TWAI interne via CanUniversal)
#define HUB_CAN_TX            GPIO_NUM_5   // convention DCC_CAN_RMT-Booster : TX=5
#define HUB_CAN_RX            GPIO_NUM_4   //                                        RX=4

// --- LED d'état
#define HUB_WS2812_PIN        GPIO_NUM_26  // 1 + HUB_NB_DISTRICTS LED