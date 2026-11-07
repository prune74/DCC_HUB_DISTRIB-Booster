// HubWatch.h — suivi nominatif des modules du bus (modèle BoosterWatch)
#pragma once
#include <Arduino.h>
#include <CanUniversal.h>

#define WATCH_MAX_MODULES 32

struct WatchedModule
{
    uint8_t type;        // 0=C, 1=B, 2=S, 3=D, 5=H
    uint8_t id;          // numéro dans son type
    uint8_t state;       // dernier état reçu : 0=OK, 1=FAULT, 2=OVERHEAT, 3=OFF
    uint32_t lastSeen;   // millis() de la dernière trame
    uint8_t faultStreak; // trames FAULT consécutives
    bool present;
};

void watchInit();
void watchFeed(const CanMsg &m);          // à appeler pour chaque 0x102
const WatchedModule *watchGet(uint8_t i); // itération 0..watchCount()-1
uint8_t watchCount();
const WatchedModule *watchFind(uint8_t type, uint8_t id);
bool watchStale(const WatchedModule &w); // pas vu depuis > WATCH_STALE_MS
void watchPrint();                       // commande série "watch"
void watchPrintJson();