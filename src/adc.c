#include "stm8s.h"
#include "adc.h"

static adcCallback measureCallback = NULL;

 //#define ADC1 ((ADC1_TypeDef *) ADC1_BaseAddress)

void adcInit(uint8_t channel, adcCallback callback)
{
    ADC1->CR1 |= ADC1_CR1_SPSEL; // fADC = fMASTER/18, recommended to change the SPSEL bits when ADC is in power down.
    ADC1->CR1 |= ADC1_CR1_CONT; // 1: Continuous conversion mode
    ADC1->CSR |= ADC1_CSR_EOCIE; // EOC interrupt enabled. An interrupt is generated when the EOC bit is set.
    ADC1->CSR &= ~ADC1_CSR_AWDIE; // 0: AWD interrupt disabled.
    ADC1->CSR |= channel & 0x0F; // 5 channel D5 pin
    ADC1->CR2 &= ~ADC1_CR2_SCAN; // 0: Scan mode disabled
    ADC1->CR2 |= ADC1_CR2_ALIGN; // 1: Right alignment (eight LSB bits are written in the ADC_DRL register then the remaining MSB bits are written in the ADC_DH register).
    // ADC1->CR2 |= ADC1_CR2_EXTTRIG; // Conversion on external event enabled
    // ADC1->CR2 &= ~ADC1_CR2_EXTSEL; // Internal TIM1 TRGO event
    ADC1->CR3 |= ADC1_CR3_DBUF; // 1: Data buffer enabled
    //TIM1->SMCR |= TIM1_SMCR_TS; // 111: External trigger input (ETRF)
    //TIM1->CR2 |= 1 << 5; // 010: Update - The update event is selected as trigger output (TRGO)
    measureCallback = callback;
}

void adcStop(void)
{
    ADC1->CR1 &= ~ADC1_CR1_CONT;
	ADC1->CR1 &= ~ADC1_CR1_ADON;
}

void adcStart(void)
{
    ADC1->CR1 |= ADC1_CR1_CONT;
    ADC1->CR1 |= ADC1_CR1_ADON;
    ADC1->CR1 |= ADC1_CR1_ADON;
}

void adcGetBufferedData(uint16_t buff[ADC_BUFFER_SIZE])
{
    for (uint8_t i = 0; i < ADC_BUFFER_SIZE; i++)
        buff[i] = ((uint16_t *) ADC1_BaseAddress)[i];
}

uint16_t adcGetData(void)
{
    uint8_t lsb = ADC1->DRL;
    return lsb | (ADC1->DRH << 8);
}

INTERRUPT_HANDLER(ADC1_IRQHandler, 22)
{
    EMIT_CB(measureCallback);
    ADC1->CSR &= ~ADC1_CSR_EOC; // clear interrupt flag
}