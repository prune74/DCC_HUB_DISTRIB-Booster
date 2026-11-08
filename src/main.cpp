// main.cpp — hub de distribution DCC, 8 districts par carte (CFC)
#include <Arduino.h>
#include "Config.h"
#include "Pins.h"
#include "DistrictRouter.h"
#include "InjectionInput.h"
#include "DistrictManager.h"
#include "GlobalInject.h"
#include "CanHub.h"
#include "HubWatch.h"
#include "StatusLed.h"
#include "Cli.h"

void setup() {
    Serial.begin(115200);
    Serial.printf("\nHub DCC_CAN v%s — hub %c, %u districts\n",
                  HUB_VERSION, 'A' + HUB_INSTANCE, HUB_NB_DISTRICTS);

    globalInjectInit();
    routerInit();      // 8 DCC out via GPIO Matrix
    injInit();         // 8 entrées TOR directes (convertisseurs)
    dmInit();
    watchInit();      // suivi des modules du bus
    canInit();
    ledInit();
    canStartTasks();
    Cli_begin();

    Serial.println("Hub prêt. Commandes : districts [json] | hub | watch [json] | cut N | on N | inject on|off | reset");
}

void loop() {
    injPoll();
    dmTick();
    ledTick();
    Cli_task();
    vTaskDelay(pdMS_TO_TICKS(2));
}