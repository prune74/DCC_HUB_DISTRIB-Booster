// StatusLed.h — strip WS2812 : LED 0 = général, LED 1..8 = districts
#pragma once

void ledInit();
void ledTick();     // à appeler ~10 Hz