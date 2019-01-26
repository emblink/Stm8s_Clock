#include "stm8s.h"
#include "ledPanel.h"
#include "max7219.h"
//#include "adc.h"

void ledPanelInit(void)
{
  max7219Init();
  // set adc finish measurments callback
}

void ledPanelSendCommand(PanelNumber num, PanelCommand cmd)
{
  switch(cmd) {
  case PANEL_TURN_ON:
    max7219SetPanelState(num, TRUE);
    break;
  case PANEL_TURN_OFF:
    max7219SetPanelState(num, FALSE);
    break;
  case PANEL_TEST_ENABLE:
    max7219SetTestMode(num, TRUE);
    break;
  case PANEL_TEST_DISABLE:
    max7219SetTestMode(num, FALSE);
    break;
  default:
    break;
  }
}

void ledPanelSendSymbol(PanelNumber panelNumber, const uint8_t symbolData[8])
{
  max7219SendSymbol(panelNumber, symbolData);
}

void ledPanelSendString(uint8_t data[], uint16_t len)
{
  
}

bool ledPanelSetGlobalIntensity(PanelIntensity intensity)
{
  if (intensity < PANEL_INTENSITY_COUNT) {
    // start measure timer and adc
    return TRUE;
  } else if (intensity == PANEL_INTENSITY_AUTO) {
      for (uint8_t i = 0; i < PANEL_COUNT; i++)
        max7219SetIntensity(i, intensity);
    return TRUE;
  }
  return FALSE;
}

bool ledPanelSetPanelIntensity(PanelNumber num, PanelIntensity intensity)
{
  if (intensity >= PANEL_INTENSITY_COUNT)
    return FALSE;
  max7219SetIntensity(num, intensity);
  return TRUE;
}

void ledPanleClearAllPanels(void)
{
  max7219ClearAllPanels();
}

bool ledPanleClearPanel(uint8_t panelNumber)
{
  if (panelNumber >= PANEL_COUNT)
    return FALSE;
  max7219ClearPanel(panelNumber);
    return TRUE;
}



