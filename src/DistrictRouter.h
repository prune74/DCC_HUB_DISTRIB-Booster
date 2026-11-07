// DistrictRouter.h — copie DCC par GPIO Matrix, coupure directe par broche
#pragma once
#include <Arduino.h>

void routerInit();
void routerSetDistrict(uint8_t n /*0..7*/, bool on);   // on = DCC routé, off = broche à 0
bool routerGetDistrict(uint8_t n);
void routerSetAll(bool on);