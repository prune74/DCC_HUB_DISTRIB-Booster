// Config.h — seuils, délais, IDs CAN, identité du hub
#pragma once

#include "Pins.h"

#define HUB_VERSION           "0.1.0"
#define HUB_NB_DISTRICTS       8

// --- Identité du hub : 0 = hub A sur EXT1, 1 = hub B sur EXT2 (SEULE différence
//     entre les deux cartes ; flasher avec 0 ou 1 selon le hub)
#define HUB_INSTANCE           0

// Injection TOR (ligne DAC des convertisseurs, ~3,04 V = défaut)
#define INJECT_CONFIRM_MS     20     // front haut confirmé
#define INJECT_RELEASE_MS     50     // front bas confirmé

// Retry et repli global
#define RETRY_DELAY1_MS       5000
#define RETRY_DELAY2_MS       10000
#define LOCK_TO_GLOBAL_MS     30000  // verrouillé -> injection LaBox
#define COUPE_TIMEOUT_MS      10000  // filet de sécurité : injection jamais
                                     // relâchée -> forcer le retry quand même

// DAC -> LaBox (seuil LaBox 2500 mA / 0,80 = 2,52 V)
#define DAC_INJECT_VALUE      235    // ~3,04 V, ne jamais dépasser 3,3 V

// CAN (TWAI 500 kbit/s, même protocole que DCC_CAN_RMT-Booster)
#define CAN_BITRATE           500000
#define CAN_TELEMETRY_ID      0x102
#define HUB_CAN_TYPE          5      // TYPE=5 = lettre 'H' : ID_H_01 (hub A) / ID_H_02 (hub B)
                                     // — à réserver côté firmware convertisseur (nomenclature ID_X_nn)
#define TELEMETRY_PERIOD_MS   1000

// WS2812
#define WS2812_BRIGHTNESS     40

// Numéro CAN du hub dans son type (= HUB_INSTANCE)
#define HUB_CAN_ID            HUB_INSTANCE