#ifndef __ADC_H
#define __ADC_H

typedef void (*adcCallback)(void);

void adcInit(void);
void adcEnable(void);
void adcDisable(void);
void adcSetCallback(adcCallback cb);

#endif // __ADC_H