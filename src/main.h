#ifndef __MAIN_H
#define __MAIN_H
#include "stm8s_gpio.h"
#include "adc.h"

#define ENCODER_BUTTON_PORT     GPIOD
#define ENCODER_BUTTON_PIN      GPIO_PIN_2
#define ENCODER_CHANNEL_A_PORT  GPIOC
#define ENCODER_CHANNEL_B_PORT  GPIOC
#define ENCODER_CHANNEL_A_PIN   GPIO_PIN_3
#define ENCODER_CHANNEL_B_PIN   GPIO_PIN_4

#define SPI_SCK_PIN             GPIO_PIN_5
#define SPI_MOSI_PIN            GPIO_PIN_6
#define SPI_CS_PIN              GPIO_PIN_7

#define BATT_MEASURE_EN_PORT    GPIOD
#define BATT_MEASURE_EN_PIN     GPIO_PIN_4

#define ADC_BATT_CHANNEL        ADC_CHANNEL_6
#define ADC_PHOTO_CHANNEL       ADC_CHANNEL_4

#define DEBUG_PORT              GPIOA
#define DEBUG_PIN               GPIO_PIN_1

#endif // __MAIN_H