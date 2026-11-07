// CanHub.cpp — bibliothèque prune74/CanUniversal (même structure que DCC_CAN_RMT-Booster)
#include "CanHub.h"
#include "HubCanConfig.h"
#include "Config.h"
#include "DistrictManager.h"
#include "HubWatch.h"
#include <CanUniversal.h>

// 0x102 : { ID, TYPE, mA/10, mV/100, état } — le hub ne mesure ni courant ni
// tension (mA=0, mV=0) : seuls ID/TYPE/état sont significatifs.
// Les deux hubs émettent avec TYPE=5 (lettre 'H') : ID_H_01 (EXT1) et ID_H_02 (EXT2).

static HubCanConfig canConfig;

void canInit()
{
    // begin() est tolérante : un bus en erreur reste accessible (invalide),
    // sans jamais faire crasher l'application.
    CanInit::begin(canConfig);
    if (!CanBus::bus(0).isValid())
        Serial.println("[CAN] bus invalide — vérifiez le câblage !");
}

static void canRxTask(void *)
{
    CanMsg rx;
    for (;;)
    {
        if (CanBus::bus(0).receive(rx))
        {
            if (rx.id == CAN_TELEMETRY_ID && rx.dlc >= 5)
            {
                watchFeed(rx); // suivi nominatif ID_X_nn (HubWatch)
            }
        }
        vTaskDelay(pdMS_TO_TICKS(2));
    }
}

static void canTxTask(void *)
{
    for (;;)
    {
        uint8_t off = 0, locked = 0;
        for (uint8_t i = 0; i < HUB_NB_DISTRICTS; i++)
        {
            DistrictState s = dmState(i);
            if (s == DS_COUPE || s == DS_COUPE_MANUEL || s == DS_ATTENTE_RETRY)
                off++;
            if (s == DS_VERROUILLE || s == DS_INJECTION_GLOBALE)
                locked++;
        }
        CanMsg m((uint16_t)CAN_TELEMETRY_ID,
                 {HUB_CAN_ID, HUB_CAN_TYPE,
                  (uint8_t)0, (uint8_t)0,                  // mA/10, mV/100
                  (uint8_t)(locked ? 1 : (off ? 3 : 0))}); // 1=FAULT, 3=OFF, 0=OK
        CanBus::bus(0).send(m);
        vTaskDelay(pdMS_TO_TICKS(TELEMETRY_PERIOD_MS));
    }
}

void canStartTasks()
{
    xTaskCreatePinnedToCore(canRxTask, "canrx", 4096, nullptr, 2, nullptr, 0);
    xTaskCreatePinnedToCore(canTxTask, "cantx", 4096, nullptr, 2, nullptr, 0);
}