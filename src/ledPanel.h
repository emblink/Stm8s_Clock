#pragma once

#include "stdint.h"

//typedef void (*LedPanelGetSymbolCallBack)(void);

typedef enum PanelNumber {
  PANEL_0 = 0,
  PANEL_1,
  PANEL_2,
  PANEL_3,
  PANEL_COUNT,
} PanelNumber;

typedef enum PanelCommand {
  PANEL_TURN_ON,
  PANEL_TURN_OFF,
  PANEL_TEST_ENABLE,
  PANEL_TEST_DISABLE,
} PanelCommand;

typedef enum PanelIntensity {
  PANEL_INTENSITY_0 = 0x00,
  PANEL_INTENSITY_1 = 0x03,
  PANEL_INTENSITY_2 = 0x06,
  PANEL_INTENSITY_3 = 0x09,
  PANEL_INTENSITY_4 = 0x0C,
  PANEL_INTENSITY_5 = 0x0F,
  PANEL_INTENSITY_AUTO,
} PanelIntensity;
        
void ledPanelInit(void);
void ledPanelSendCommand(PanelCommand cmd, PanelNumber num);
void ledPanelSendSymbol(uint8_t symbol, PanelNumber num);
void ledPanelSendString(uint8_t data[], uint16_t len);
void ladPanelSetIntensity(PanelIntensity value, PanelNumber num);