#ifndef __LED_PANEL_H
#define __LED_PANEL_H

#include "stm8s.h"

typedef enum PanelNumber {
  PANEL_0 = 0,
  PANEL_1,
  PANEL_2,
  PANEL_3,
  PANEL_COUNT,
} PanelNumber;

typedef enum PanelCommand {
  PANEL_TURN_OFF = 0x00,
  PANEL_TURN_ON,
  PANEL_TEST_DISABLE,
  PANEL_TEST_ENABLE,
} PanelCommand;

typedef enum PanelIntensity {
  PANEL_INTENSITY_0 = 0x00,
  PANEL_INTENSITY_1 = 0x01,
  PANEL_INTENSITY_2 = 0x02,
  PANEL_INTENSITY_3 = 0x03,
  PANEL_INTENSITY_4 = 0x04,
  PANEL_INTENSITY_5 = 0x05,
  PANEL_INTENSITY_6 = 0x06,
  PANEL_INTENSITY_7 = 0x07,
  PANEL_INTENSITY_8 = 0x08,
  PANEL_INTENSITY_9 = 0x09,
  PANEL_INTENSITY_10 = 0x0A,
  PANEL_INTENSITY_11 = 0x0B,
  PANEL_INTENSITY_12 = 0x0C,
  PANEL_INTENSITY_13 = 0x0D,
  PANEL_INTENSITY_14 = 0x0E,
  PANEL_INTENSITY_15 = 0x0F,
  PANEL_INTENSITY_COUNT,
  PANEL_INTENSITY_AUTO, // enabled only for all ledPanels
} PanelIntensity;
        
void ledPanelInit(void);
void ledPanelSendCommand(PanelNumber num, PanelCommand cmd);
void ledPanelSendSymbol(PanelNumber num, const uint8_t symbol[8]);
void ledPanelSendString(uint8_t data[], uint16_t len);
bool ledPanelSetGlobalIntensity(PanelIntensity intensity);
bool ledPanelSetPanelIntensity(PanelNumber num, PanelIntensity intensity);
void ledPanleClearAllPanels(void);
bool ledPanleClearPanel(uint8_t panelNumber);

#endif // __LED_PANEL_H