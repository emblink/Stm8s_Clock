#ifndef __MAIN_H
#define __MAIN_H
#include "stm8s_gpio.h"

#define RED_LED_PIN             GPIO_PIN_3
#define BUTTON_PIN              GPIO_PIN_4
#define GREEN_LED_PIN           GPIO_PIN_3
#define SPI_CS_PIN              GPIO_PIN_7
#define SPI_MOSI_PIN            GPIO_PIN_6
#define SPI_SCK_PIN             GPIO_PIN_5
#define ENCODER_CHANNEL_A_PIN   GPIO_PIN_1
#define ENCODER_CHANNEL_B_PIN   GPIO_PIN_2

#endif // __MAIN_H

/* PINMAP

PORT_C

#define RED_LED_PIN GPIOC_PIN_3

#define ENCODER_BUTTON_PIN GPIOC_PIN_4 // make pull up

#define SPI_SCK_PIN GPIOC_PIN_5

#define SPI_MOSI_PIN GPIOC_PIN_6

#define SPI_CS_PIN GPIOC_PIN_7

PORT_D

#define ENCODER_CHANNEL_1 GPIOD_PIN_1

#define ENCODER_CHANNEL_2 GPIOD_PIN_2

#define GREEN_LED_PIN GPIOD_PIN_3

PORT_B

#define I2C_SCL_PIN GPIOB_PIN_4

#define I2C_SDA_PIN GPIOB_PIN_5

*/