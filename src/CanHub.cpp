// CanHub.cpp — même protocole que DCC_CAN_RMT-Booster, bus commun aux 2 hubs
#include "CanHub.h"
#include "Config.h"
#include "Pins.h"
#include "DistrictManager.h"
#include "driver/twai.h"

// 0x102 : { ID, TYPE, mA/10, mV/100, état } — le hub ne mesure ni courant ni
// tension (mA=0, mV=0) : seuls ID/TYPE/état sont significatifs.
// Les deux hubs émettent avec TYPE=5 (lettre 'H') : ID_H_01 (EXT1) et ID_H_02 (EXT2).
void canInit() {
    twai_config_t g = TWAI_GENERAL_CONFIG_DEFAULT(HUB_CAN_TX_PIN, HUB_CAN_RX_PIN, TWAI_MODE_NORMAL);
    g.rx_queue_len = 32;
    twai_timing_config_t t = TWAI_TIMING_CONFIG_500KBITS();
    twai_filter_config_t f = TWAI_FILTER_CONFIG_ACCEPT_ALL();
    twai_driver_install(&g, &t, &f);
    twai_start();
}

static void canRxTask(void*) {
    twai_message_t msg;
    for (;;) {
        if (twai_receive(&msg, pdMS_TO_TICKS(100)) == ESP_OK) {
            if (msg.identifier == CAN_TELEMETRY_ID && msg.data_length_code >= 5) {
                // uint8_t id = msg.data[0], type = msg.data[1], etat = msg.data[4];
                // TODO : suivi BoosterWatch-style des ID_B_xx pour croiser
                // injections TOR et états CAN (diagnostic, journalisation).
            }
        }
    }
}

static void canTxTask(void*) {
    for (;;) {
        uint8_t off = 0, locked = 0;
        for (uint8_t i = 0; i < HUB_NB_DISTRICTS; i++) {
            DistrictState s = dmState(i);
            if (s == DS_COUPE || s == DS_COUPE_MANUEL || s == DS_ATTENTE_RETRY) off++;
            if (s == DS_VERROUILLE || s == DS_INJECTION_GLOBALE) locked++;
        }
        twai_message_t m = {};
        m.identifier = CAN_TELEMETRY_ID;
        m.data_length_code = 5;
        m.data[0] = HUB_CAN_ID;
        m.data[1] = HUB_CAN_TYPE;
        m.data[2] = 0;  // mA/10
        m.data[3] = 0;  // mV/100
        m.data[4] = locked ? 1 : (off ? 3 : 0);  // 1=FAULT, 3=OFF, 0=OK
        twai_transmit(&m, pdMS_TO_TICKS(10));
        vTaskDelay(pdMS_TO_TICKS(TELEMETRY_PERIOD_MS));
    }
}

void canStartTasks() {
    xTaskCreatePinnedToCore(canRxTask, "canrx", 4096, nullptr, 2, nullptr, 0);
    xTaskCreatePinnedToCore(canTxTask, "cantx", 4096, nullptr, 2, nullptr, 0);
}