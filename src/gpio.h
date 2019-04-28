#ifndef __GPIO_H
#define __GPIO_H

#include "stm8s.h"

typedef enum GpioPin {
    GPIO_PIN_0    = ((uint8_t)0x01),  /*!< Pin 0 selected */
    GPIO_PIN_1    = ((uint8_t)0x02),  /*!< Pin 1 selected */
    GPIO_PIN_2    = ((uint8_t)0x04),  /*!< Pin 2 selected */
    GPIO_PIN_3    = ((uint8_t)0x08),   /*!< Pin 3 selected */
    GPIO_PIN_4    = ((uint8_t)0x10),  /*!< Pin 4 selected */
    GPIO_PIN_5    = ((uint8_t)0x20),  /*!< Pin 5 selected */
    GPIO_PIN_6    = ((uint8_t)0x40),  /*!< Pin 6 selected */
    GPIO_PIN_7    = ((uint8_t)0x80),  /*!< Pin 7 selected */
} GpioPin;

typedef enum GpioMode {
  GPIO_MODE_IN_FL_NO_IT      = (uint8_t)0x00,  /*!< Input floating, no external interrupt */
  GPIO_MODE_IN_PU_NO_IT      = (uint8_t)0x40,  /*!< Input pull-up, no external interrupt */
  GPIO_MODE_IN_FL_IT         = (uint8_t)0x20,  /*!< Input floating, external interrupt */
  GPIO_MODE_IN_PU_IT         = (uint8_t)0x60,  /*!< Input pull-up, external interrupt */
  GPIO_MODE_OUT_OD_LOW_FAST  = (uint8_t)0xA0,  /*!< Output open-drain, low level, 10MHz */
  GPIO_MODE_OUT_PP_LOW_FAST  = (uint8_t)0xE0,  /*!< Output push-pull, low level, 10MHz */
  GPIO_MODE_OUT_OD_LOW_SLOW  = (uint8_t)0x80,  /*!< Output open-drain, low level, 2MHz */
  GPIO_MODE_OUT_PP_LOW_SLOW  = (uint8_t)0xC0,  /*!< Output push-pull, low level, 2MHz */
  GPIO_MODE_OUT_OD_HIZ_FAST  = (uint8_t)0xB0,  /*!< Output open-drain, high-impedance level,10MHz */
  GPIO_MODE_OUT_PP_HIGH_FAST = (uint8_t)0xF0,  /*!< Output push-pull, high level, 10MHz */
  GPIO_MODE_OUT_OD_HIZ_SLOW  = (uint8_t)0x90,  /*!< Output open-drain, high-impedance level, 2MHz */
  GPIO_MODE_OUT_PP_HIGH_SLOW = (uint8_t)0xD0   /*!< Output push-pull, high level, 2MHz */
} GpioMode;

typedef enum BspPin {
    RED_LED_PIN,
    GREEN_LED_PIN,
    SPI_CS_PIN,
    SPI_MOSI_PIN,
    SPI_SCK_PIN,
    ENCODER_BUTTON_PIN,
    ENCODER_CHANNEL_A_PIN,
    ENCODER_CHANNEL_B_PIN,
    BSP_PIN_COUNT
} BspPin;

void gpioPinInit(BspPin pin, GpioMode mode);
void gpioWritePin(BspPin pin, bool state);
void gpioTogglePin(BspPin pin);
bool gpioReadPin(BspPin pin);

#endif // __GPIO_H