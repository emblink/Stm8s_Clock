#ifndef __ADC_H
#define __ADC_H

#include "stm8s.h"

#define EMIT_CB(cb) {if (cb) {cb();}}
#define ADC_BUFFER_SIZE (10)

typedef void (*adcCallback)(void);

void adcInit(uint8_t channel, adcCallback callback);
void adcStart(void);
void adcStop(void);
void adcGetBufferedData(uint16_t buff[ADC_BUFFER_SIZE]);
uint16_t adcGetData(void);

#endif // __ADC_H