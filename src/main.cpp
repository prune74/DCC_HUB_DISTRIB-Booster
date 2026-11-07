// main.cpp — hub de distribution DCC, 8 districts par carte (CFC)
#include <Arduino.h>
#include <Wire.h>
#include "Config.h"
#include "Pins.h"
#include "DistrictRouter.h"
#include "InjectionInput.h"
#include "DistrictManager.h"
#include "GlobalInject.h"
#include "CanHub.h"
#include "StatusLed.h"
#include "Cli.h"

void setup() {
    Serial.begin(115200);
    Serial.printf("\nHub DCC_CAN v%s — hub %c, %u districts\n",
                  HUB_VERSION, 'A' + HUB_INSTANCE, HUB_NB_DISTRICTS);

    Wire.begin(HUB_I2C_SDA, HUB_I2C_SCL, 400000);
    globalInjectInit();
    routerInit();      // 8 DCC out via GPIO Matrix
    injInit();         // MCP23017 + interruption
    dmInit();
    canInit();
    ledInit();
    canStartTasks();
    cliInit();

    Serial.println("Hub prêt. Commandes : districts [json] | hub | cut N | on N | inject on|off | reset");
}

void loop() {
    injPoll();
    dmTick();
    ledTick();
    vTaskDelay(pdMS_TO_TICKS(2));
}