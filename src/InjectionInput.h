// InjectionInput.h — lecture des 8 injections TOR via MCP23017 (interruption)
#pragma once
#include <Arduino.h>

void injInit();
void injPoll();                 // appelé en boucle rapide, lit sur INT
bool injActive(uint8_t n);      // niveau TOR courant du district n (0..7)