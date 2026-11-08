// HubWatch.cpp — même logique que BoosterWatch du convertisseur :
// un module suivi individuellement (nom ID_X_nn, état, dernière vue),
// indépendant des injections TOR lues sur GPIO (croisement au diagnostic).
#include "HubWatch.h"
#include "Config.h"

#define WATCH_STALE_MS  3000

static WatchedModule modules[WATCH_MAX_MODULES];
static uint8_t count = 0;

static const char typeLetter(uint8_t type) {
    switch (type) {
        case 0: return 'C'; case 1: return 'B'; case 2: return 'S';
        case 3: return 'D'; case 5: return 'H'; default: return '?';
    }
}

void watchInit() {
    count = 0;
    for (auto& m : modules) m = {};
}

void watchFeed(const CanMsg& m) {
    if (m.dlc < 5) return;             // DLC 4 (anciens boosters) : type B implicite
    uint8_t type = (m.dlc >= 5) ? m.data[1] : 1;
    uint8_t id   = m.data[0];
    uint8_t state = (m.dlc >= 5) ? m.data[4] : m.data[3];

    WatchedModule* slot = nullptr;
    for (uint8_t i = 0; i < count; i++)
        if (modules[i].type == type && modules[i].id == id) { slot = &modules[i]; break; }
    if (!slot && count < WATCH_MAX_MODULES) { slot = &modules[count++]; slot->type = type; slot->id = id; }
    if (!slot) return;

    slot->lastSeen = millis();
    slot->present  = true;
    if (state == 1 || state == 2) slot->faultStreak++;
    else slot->faultStreak = 0;
    slot->state = state;
}

const WatchedModule* watchGet(uint8_t i) { return (i < count) ? &modules[i] : nullptr; }
uint8_t watchCount() { return count; }

const WatchedModule* watchFind(uint8_t type, uint8_t id) {
    for (uint8_t i = 0; i < count; i++)
        if (modules[i].type == type && modules[i].id == id) return &modules[i];
    return nullptr;
}

bool watchStale(const WatchedModule& w) { return millis() - w.lastSeen > WATCH_STALE_MS; }

void watchPrint() {
    Serial.printf("modules vus sur le bus : %u\n", count);
    for (uint8_t i = 0; i < count; i++) {
        const WatchedModule& w = modules[i];
        const char* stateName[] = {"OK","FAULT","OVERHEAT","OFF"};
        Serial.printf("ID_%c_%02u  %-9s vu il y a %lu ms%s\n",
                      typeLetter(w.type), w.id, stateName[w.state & 3],
                      (unsigned long)(millis() - w.lastSeen),
                      watchStale(w) ? "  [STALE]" : "");
    }
}

void watchPrintJson() {
    for (uint8_t i = 0; i < count; i++) {
        const WatchedModule& w = modules[i];
        Serial.printf("{\"type\":%u,\"id\":%u,\"state\":%u,\"age_ms\":%lu},\n",
                      w.type, w.id, w.state, (unsigned long)(millis() - w.lastSeen));
    }
}