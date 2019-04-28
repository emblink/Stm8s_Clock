#include "stm8s.h"
#include "gpio.h"

typedef struct GpioPinStruct {
    GPIO_TypeDef * port;
    GpioPin pin;
} GpioPinStruct;

static GpioPinStruct BspPinMap[BSP_PIN_COUNT] = {
    [RED_LED_PIN]           = {.port = GPIOC, .pin = GPIO_PIN_3},
    [GREEN_LED_PIN]         = {.port = GPIOD, .pin = GPIO_PIN_3},
    [SPI_CS_PIN]            = {.port = GPIOC, .pin = GPIO_PIN_7},
    [SPI_MOSI_PIN]          = {.port = GPIOC, .pin = GPIO_PIN_6},
    [SPI_SCK_PIN]           = {.port = GPIOC, .pin = GPIO_PIN_5},
    [ENCODER_BUTTON_PIN]    = {.port = GPIOC, .pin = GPIO_PIN_4},
    [ENCODER_CHANNEL_A_PIN] = {.port = GPIOA, .pin = GPIO_PIN_1},
    [ENCODER_CHANNEL_B_PIN] = {.port = GPIOA, .pin = GPIO_PIN_2},
};

void gpioPinInit(BspPin pin, GpioMode mode)
{  
  /* Reset corresponding bit to GPIO_Pin in CR2 register */
  BspPinMap[pin].port->CR2 &= (uint8_t)(~BspPinMap[pin].pin);
  
  /*-----------------------------*/
  /* Input/Output mode selection */
  /*-----------------------------*/
  
  if ((((uint8_t)(mode)) & (uint8_t)0x80) != (uint8_t)0x00) /* Output mode */
  {
        if ((((uint8_t)(mode)) & (uint8_t)0x10) != (uint8_t)0x00) /* High level */
        {
          BspPinMap[pin].port->ODR |= (uint8_t)BspPinMap[pin].pin;
        } 
        else /* Low level */
        {
          BspPinMap[pin].port->ODR &= (uint8_t)(~BspPinMap[pin].pin);
        }
        /* Set Output mode */
        BspPinMap[pin].port->DDR |= (uint8_t)BspPinMap[pin].pin;
  } 
  else /* Input mode */
  {
        /* Set Input mode */
        BspPinMap[pin].port->DDR &= (uint8_t)(~BspPinMap[pin].pin);
  }
  
  /*------------------------------------------------------------------------*/
  /* Pull-Up/Float (Input) or Push-Pull/Open-Drain (Output) modes selection */
  /*------------------------------------------------------------------------*/
  
  if ((((uint8_t)(mode)) & (uint8_t)0x40) != (uint8_t)0x00) /* Pull-Up or Push-Pull */
  {
      BspPinMap[pin].port->CR1 |= (uint8_t)BspPinMap[pin].pin;
  } 
  else /* Float or Open-Drain */
  {
      BspPinMap[pin].port->CR1 &= (uint8_t)(~BspPinMap[pin].pin);
  }
  
  /*-----------------------------------------------------*/
  /* Interrupt (Input) or Slope (Output) modes selection */
  /*-----------------------------------------------------*/
  
  if ((((uint8_t)(mode)) & (uint8_t)0x20) != (uint8_t)0x00) /* Interrupt or Slow slope */
  {
      BspPinMap[pin].port->CR2 |= (uint8_t)BspPinMap[pin].pin;
  } 
  else /* No external interrupt or No slope control */
  {
      BspPinMap[pin].port->CR2 &= (uint8_t)(~BspPinMap[pin].pin);
  }
}

void gpioWritePin(BspPin pin, bool state)
{
    if (state)
        BspPinMap[pin].port->ODR |= BspPinMap[pin].pin;
    else
        BspPinMap[pin].port->ODR &= ~BspPinMap[pin].pin;
}

void gpioTogglePin(BspPin pin)
{
    BspPinMap[pin].port->ODR ^= BspPinMap[pin].pin;
}

bool gpioReadPin(BspPin pin)
{
    return BspPinMap[pin].port->IDR & BspPinMap[pin].pin ? TRUE : FALSE;
}

/* PINMAP

PORT_C

#define RED_LED_PIN GPIOC_PIN_3

#define ENCODER_BUTTON_PIN GPIOC_PIN_4 // make pull up

#define SPI_SCK_PIN GPIOC_PIN_5

#define SPI_MOSI_PIN GPIOC_PIN_6

#define SPI_CS_PIN GPIOC_PIN_7

PORT_D

#define ENCODER_CHANNEL_1 GPIOA_PIN_1

#define ENCODER_CHANNEL_2 GPIOA_PIN_2

#define GREEN_LED_PIN GPIOD_PIN_3

PORT_B

#define I2C_SCL_PIN GPIOB_PIN_4

#define I2C_SDA_PIN GPIOB_PIN_5

*/