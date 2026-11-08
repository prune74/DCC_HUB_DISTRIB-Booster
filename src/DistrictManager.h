// DistrictManager.h — machine à états des 8 districts
#pragma once
#include <Arduino.h>

enum DistrictState : uint8_t {
    DS_ACTIF = 0,
    DS_COUPE,
    DS_ATTENTE_RETRY,
    DS_VERROUILLE,
    DS_COUPE_MANUEL,
    DS_INJECTION_GLOBALE
};

void dmInit();
void dmTick();                  // à appeler en boucle (~2 ms)
DistrictState dmState(uint8_t n);
const char* dmStateName(uint8_t n);
void dmCut(uint8_t n);          // commande manuelle
void dmOn(uint8_t n);           // réactivation manuelle (RAZ retries)