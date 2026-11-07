// GlobalInject.h — DAC GPIO25 -> broche intensité de l'EXT du hub
#pragma once

void globalInjectInit();
void globalInject(bool on);
bool globalInjectActive();