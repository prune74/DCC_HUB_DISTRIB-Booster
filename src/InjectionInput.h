// InjectionInput.h — lecture des 8 injections TOR sur GPIO directs
#pragma once
#include <Arduino.h>

void injInit();
void injPoll();                 // appelé en boucle rapide (~2 ms), relit les 8 broches
bool injActive(uint8_t n);      // niveau TOR courant du district n (0..7)