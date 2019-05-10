#ifndef __ADC_H
#define __ADC_H

#include "stm8s.h"

typedef void (*adcCallback)(void);

void adcInit(uint8_t channel, adcCallback callback);
void adcEnable(void);
void adcDisable(void);

#endif // __ADC_H