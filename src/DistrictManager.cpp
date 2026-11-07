// DistrictManager.cpp — niveaux 1.5 (coupe sélective), 2 (retry), 3 (repli global)
#include "DistrictManager.h"
#include "Config.h"
#include "DistrictRouter.h"
#include "InjectionInput.h"
#include "GlobalInject.h"

struct District {
    DistrictState state;
    uint32_t since;         // dernier changement d'état
    uint32_t highSince;     // premier front haut (anti-rebond)
    uint32_t lowSince;      // premier front bas confirmé
    uint8_t  retries;
};

static District d[HUB_NB_DISTRICTS];
static bool lastLevel[HUB_NB_DISTRICTS];

static const uint32_t retryDelays[] = { RETRY_DELAY1_MS, RETRY_DELAY2_MS };

static void setState(uint8_t i, DistrictState s) {
    if (d[i].state != s) {
        Serial.printf("[district %u] %s -> %s\n", i + 1,
                      dmStateName(i), dmStateName(i));  // cf. remarque bas de fichier
        d[i].state = s; d[i].since = millis();
    }
}

void dmInit() {
    for (uint8_t i = 0; i < HUB_NB_DISTRICTS; i++) {
        d[i] = {DS_ACTIF, 0, 0, 0, 0};
        lastLevel[i] = false;
    }
}

void dmTick() {
    uint32_t now = millis();
    for (uint8_t i = 0; i < HUB_NB_DISTRICTS; i++) {
        bool inj = injActive(i);
        bool edge = (inj != lastLevel[i]);
        lastLevel[i] = inj;

        switch (d[i].state) {
        case DS_ACTIF:
            if (inj) {
                if (edge || d[i].highSince == 0) d[i].highSince = now;
                if (now - d[i].highSince >= INJECT_CONFIRM_MS) {
                    routerSetDistrict(i, false);           // niveau 1.5
                    setState(i, DS_COUPE);
                    d[i].lowSince = 0;
                }
            } else d[i].highSince = 0;
            break;

        case DS_COUPE:
            if (!inj) {
                if (edge || d[i].lowSince == 0) d[i].lowSince = now;
                if (now - d[i].lowSince >= INJECT_RELEASE_MS) {
                    d[i].retries++;
                    if (d[i].retries <= sizeof(retryDelays) / sizeof(retryDelays[0]))
                        setState(i, DS_ATTENTE_RETRY);
                    else
                        setState(i, DS_VERROUILLE);
                }
            } else d[i].lowSince = 0;
            break;

        case DS_ATTENTE_RETRY: {
            uint8_t k = d[i].retries - 1;
            if (k >= sizeof(retryDelays) / sizeof(retryDelays[0]))
                k = sizeof(retryDelays) / sizeof(retryDelays[0]) - 1;
            if (now - d[i].since >= retryDelays[k]) {
                routerSetDistrict(i, true);                // niveau 2
                d[i].highSince = 0; d[i].lowSince = 0;
                setState(i, DS_ACTIF);
            }
            break;
        }

        case DS_VERROUILLE:
            if (now - d[i].since >= LOCK_TO_GLOBAL_MS && !globalInjectActive()) {
                globalInject(true);                        // niveau 3
                setState(i, DS_INJECTION_GLOBALE);
            }
            break;

        case DS_INJECTION_GLOBALE:
            if (!inj) {
                globalInject(false);                       // tout est revenu OK
                setState(i, DS_VERROUILLE);
            }
            break;

        case DS_COUPE_MANUEL:
            break;   // ne réagit pas aux injections : action humaine volontaire
        }
    }
}

DistrictState dmState(uint8_t n) { return (n < HUB_NB_DISTRICTS) ? d[n].state : DS_ACTIF; }

const char* dmStateName(uint8_t n) {
    static const char* names[] = {"ACTIF","COUPE","ATTENTE_RETRY","VERROUILLE","COUPE_MANUEL","INJECTION_GLOBALE"};
    return names[(n < HUB_NB_DISTRICTS) ? d[n].state : DS_ACTIF];
}

void dmCut(uint8_t n) { if (n < HUB_NB_DISTRICTS) { routerSetDistrict(n, false); setState(n, DS_COUPE_MANUEL); } }
void dmOn(uint8_t n)  { if (n < HUB_NB_DISTRICTS) { d[n].retries = 0; routerSetDistrict(n, true); setState(n, DS_ACTIF); } }