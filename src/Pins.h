// Pins.h — brochage du hub de distribution (8 districts)
// Toutes les broches sont de type gpio_num_t (GPIO_NUM_x), cohérent avec
// CanUniversal et les API driver/gpio. Les API Arduino (pinMode, digitalWrite,
// dacWrite, pinMode…) acceptent ces valeurs directement.
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

// --- Injections TOR des convertisseurs : 8 entrées directes (sans MCP23017).
//     Lignes toujours pilotées par le DAC du convertisseur (0 V ou ~3,04 V via
//     résistance série 4,7-10 kΩ) : aucun pull-up nécessaire — c'est pour cela
//     que les entrées "input only" (34, 35, 36, 39) conviennent ici.
//     Au boot, tous les convertisseurs sortent 0 V -> lignes LOW, aucun faux défaut.
#define HUB_INJ_PINS         { GPIO_NUM_21, GPIO_NUM_22, GPIO_NUM_23, \
                               GPIO_NUM_33, GPIO_NUM_34, GPIO_NUM_35, \
                               GPIO_NUM_36, GPIO_NUM_39 }

// --- CAN (TWAI interne via CanUniversal)
#define HUB_CAN_TX            GPIO_NUM_5   // convention DCC_CAN_RMT-Booster : TX=5
#define HUB_CAN_RX            GPIO_NUM_4   //                                        RX=4

// --- LED d'état
#define HUB_WS2812_PIN        GPIO_NUM_26  // 1 + HUB_NB_DISTRICTS LED