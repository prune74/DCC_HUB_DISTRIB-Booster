// Cli.cpp — mêmes commandes que DCC_CAN_RMT-Booster + gestion des districts
#include "Cli.h"
#include "Config.h"
#include "DistrictManager.h"
#include "GlobalInject.h"

void cliInit() {
    xTaskCreatePinnedToCore([](void*) {
        String line;
        for (;;) {
            while (Serial.available()) line += (char)Serial.read();
            if (line.endsWith("\n") || line.endsWith("\r")) {
                line.trim();
                if (line == "districts") {
                    Serial.printf("hub %c (EXT%u) :\n", 'A' + HUB_INSTANCE, HUB_INSTANCE + 1);
                    for (uint8_t i = 0; i < HUB_NB_DISTRICTS; i++)
                        Serial.printf("  %2u : %s\n", i + 1, dmStateName(i));
                } else if (line == "districts json") {
                    for (uint8_t i = 0; i < HUB_NB_DISTRICTS; i++)
                        Serial.printf("{\"hub\":%u,\"district\":%u,\"state\":\"%s\"},\n",
                                      HUB_INSTANCE, i + 1, dmStateName(i));
                } else if (line == "hub") {
                    Serial.printf("hub %c, ID CAN %u, version %s\n",
                                  'A' + HUB_INSTANCE, HUB_CAN_ID, HUB_VERSION);
                } else if (line.startsWith("cut ")) {
                    int n = line.substring(4).toInt();
                    if (n >= 1 && n <= HUB_NB_DISTRICTS) dmCut(n - 1);
                } else if (line.startsWith("on ")) {
                    int n = line.substring(3).toInt();
                    if (n >= 1 && n <= HUB_NB_DISTRICTS) dmOn(n - 1);
                } else if (line == "inject on") {
                    globalInject(true);
                } else if (line == "inject off") {
                    globalInject(false);
                } else if (line == "reset") {
                    ESP.restart();
                } else {
                    Serial.println("commandes : districts [json] | hub | cut N | on N | inject on|off | reset");
                }
                line = "";
            }
            vTaskDelay(pdMS_TO_TICKS(20));
        }
    }, "cli", 4096, nullptr, 1, nullptr, 0);
}