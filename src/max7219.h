#ifndef __MAX_7219_H
#define __MAX_7219_H

#include "stm8s.h"
#define PANEL_COUNT (4)

void max7219Init(void);
void max7219SetTestMode(uint8_t panleIndex, bool testEnable);
void max7219SetPanelState(uint8_t panleIndex, bool panelEnable);
void max7219SetIntensity(uint8_t panleIndex, uint8_t intensity);
void max7219SetAutoIntensity(void);
void max7219SendSymbol(uint8_t num, const uint8_t data[8]);
void max7219ClearAllPanels(void);
void max7219ClearPanel(uint8_t panelNumber);

#endif // __MAX_7219_H