#include "stm8s.h"
#include "adc.h"

#define EMIT_CB(cb) {if (cb) {cb();}}

static adcCallback measureCallback = NULL;

void adcInit(void)
{
	
}

void adcEnable(void)
{
	
}

void adcDisable(void)
{
	
}

void adcSetCallback(adcCallback cb)
{
	
}

// interrupt
// EMIT_CB(measureCallback);