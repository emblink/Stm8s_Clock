#include "stm8s.h"
#include "adc.h"

#define EMIT_CB(cb) {if (cb) {cb();}}

static adcCallback measureCallback = NULL;

void adcInit(uint8_t channel, adcCallback callback)
{
    ADC1->CR1 |= ADC1_CR1_SPSEL; // fADC = fMASTER/18, recommended to change the SPSEL bits when ADC is in power down.
    ADC1->CR1 |= ADC1_CR1_CONT; // 1: Continuous conversion mode
    ADC1->CSR |= ADC1_CSR_EOCIE; // EOC interrupt enabled. An interrupt is generated when the EOC bit is set.
    ADC1->CSR &= ~ADC1_CSR_AWDIE; // 0: AWD interrupt disabled.
    ADC1->CSR |= channel & 0x0F; // 5 channel D5 pin
    ADC1->CR2 &= ~ADC1_CR2_SCAN; // 0: Scan mode disabled
    ADC1->CR2 &= ~ADC1_CR2_ALIGN; // 0: Left alignment
    ADC1->CR2 |= ADC1_CR2_EXTTRIG; // Conversion on external event enabled
    ADC1->CR2 &= ~ADC1_CR2_EXTSEL; //  Internal TIM1 TRGO event
    ADC1->CR3 |= ADC1_CR3_DBUF; // 1: Data buffer enabled
    TIM1->SMCR |= TIM1_SMCR_TS; // 111: External trigger input (ETRF)
    measureCallback = callback;
}

void adcEnable(void)
{
	ADC1->CR1 |= ADC1_CR1_ADON; // wakes up the ADC from power down mode
}

void adcDisable(void)
{
	ADC1->CR1 &= ~ADC1_CR1_ADON;
}

INTERRUPT_HANDLER(ADC1_IRQHandler, 22)
{
    EMIT_CB(measureCallback);
}