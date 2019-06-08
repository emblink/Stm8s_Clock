#ifndef __ADC_H
#define __ADC_H

#include "stm8s.h"

#define EMIT_CB(cb) {if (cb) {cb();}}
#define ADC_BUFFER_SIZE (10)

typedef enum AdcChannel {
    ADC_CHANNEL_0, // PC4
    ADC_CHANNEL_1, // PD2
    ADC_CHANNEL_2, // PD3
    ADC_CHANNEL_3, // PD5
    ADC_CHANNEL_4, // PD6
    ADC_CHANNEL_COUNT
} AdcChannel;

typedef void (*adcCallback)(void);

void adcInit(void);
bool adcStartMesurment(AdcChannel channel, adcCallback callback);
void adcGetBufferedData(uint16_t buff[ADC_BUFFER_SIZE]); // get data in callback
void adcStop(void);
uint16_t adcGetData(void);

#endif // __ADC_H