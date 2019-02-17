#include "main.h"
#include "stm8s.h"
#include "stm8s_gpio.h"
#include "stm8s_exti.h"
#include "stm8s_tim1.h"
#include "stm8s_clk.h"
#include "stm8s_spi.h"
#include "stm8s_i2c.h"
#include "font.h"
#include "max7219.h"
#include "DS1307.h"
#include "i2c.h"

//#define DEBUG
#define PANEL_UPDATE_PERIOD_MS 40

static uint16_t getCurrentTick(void);
static void delayMs(uint16_t delay);

static volatile uint16_t timeTick = 0;
static uint8_t encoderCounter = 0;

int main( void )
{
	/* Configure system clock */
	CLK_HSICmd(ENABLE);
	while (!CLK->ICKR & (1 << 1)); // wait untill clock became stable
	CLK_SYSCLKConfig(CLK_PRESCALER_HSIDIV1); // 16MHz SYS
	CLK_SYSCLKConfig(CLK_PRESCALER_CPUDIV1); // 16MHz CPU

	/* Enable clocks for peripherals */
	CLK_PeripheralClockConfig(CLK_PERIPHERAL_TIMER1, ENABLE);
	CLK_PeripheralClockConfig(CLK_PERIPHERAL_ADC, ENABLE);
	CLK_PeripheralClockConfig(CLK_PERIPHERAL_UART1, ENABLE);
	CLK_PeripheralClockConfig(CLK_PERIPHERAL_I2C, ENABLE);
	CLK_PeripheralClockConfig(CLK_PERIPHERAL_SPI, ENABLE);

	/* Init Leds */
	GPIO_Init(GPIOC, RED_LED_PIN, GPIO_MODE_OUT_PP_LOW_SLOW); // C3
	GPIO_Init(GPIOD, GREEN_LED_PIN, GPIO_MODE_OUT_PP_LOW_SLOW); // D3
	
	/* Init Encoder */
	GPIO_Init(GPIOC, BUTTON_PIN, GPIO_MODE_IN_PU_IT); // C4
	EXTI_SetExtIntSensitivity(EXTI_PORT_GPIOC, EXTI_SENSITIVITY_RISE_FALL);
	GPIO_Init(GPIOA, ENCODER_CHANNEL_A_PIN, GPIO_MODE_IN_FL_IT); // A1
	EXTI_SetExtIntSensitivity(EXTI_PORT_GPIOA, EXTI_SENSITIVITY_RISE_FALL);
	GPIO_Init(GPIOA, ENCODER_CHANNEL_B_PIN, GPIO_MODE_IN_FL_NO_IT); // A2
	
	/* Init SPI */
	GPIO_Init(GPIOC, SPI_CS_PIN, GPIO_MODE_OUT_PP_HIGH_SLOW);
	GPIO_Init(GPIOC, SPI_MOSI_PIN, GPIO_MODE_OUT_PP_HIGH_FAST);
	GPIO_Init(GPIOC, SPI_SCK_PIN, GPIO_MODE_OUT_PP_HIGH_FAST);
	
	SPI_DeInit();
	SPI_Init(SPI_FIRSTBIT_MSB, SPI_BAUDRATEPRESCALER_2, SPI_MODE_MASTER,
		   	 SPI_CLOCKPOLARITY_LOW, SPI_CLOCKPHASE_1EDGE,
			 SPI_DATADIRECTION_1LINE_TX, SPI_NSS_SOFT, 0x07);
	SPI_CalculateCRCCmd(DISABLE);
	SPI_Cmd(ENABLE);
	
	/* Init I2c */
	i2cInit();
	
	/* Timer 1 Init */
	TIM1_DeInit();
	TIM1_PrescalerConfig(15, TIM1_PSCRELOADMODE_UPDATE); // 1599 + 1 = 1600
	TIM1_SetAutoreload(1000); // frequency = 16M / 16 / 1000 = 1000 Hz
	TIM1_ITConfig(TIM1_IT_UPDATE, ENABLE);
	TIM1_CounterModeConfig(TIM1_COUNTERMODE_UP);

	/* Max7219 Init */
	static uint8_t max7219Buff[MAX7219_BUFF_SIZE];
	max7219Init(max7219Buff, sizeof(max7219Buff));
	
	uint16_t lastTick = 0;
	enableInterrupts();
	
	/* Start application timer */
	TIM1_Cmd(ENABLE);
	
	ds1307_reset();
	ds1307_set_hours(1);
	ds1307_set_minutes(15);
	ds1307_set_seconds(40);
	
	while(1)
	{
		if (getCurrentTick() - lastTick > PANEL_UPDATE_PERIOD_MS) {
			GPIO_WriteReverse(GPIOD, GREEN_LED_PIN);
			max7219SendSymbol(MAX7219_NUMBER_COUNT, fontGetSpaceArray());
			max7219SendSymbol(MAX7219_NUMBER_0, fontGetNumberArray(encoderCounter % 10));
			lastTick = getCurrentTick();
		}
		//wfi();
	}
}

static uint16_t getCurrentTick(void)
{ 
  uint16_t tick = 0;
  disableInterrupts();
  tick = timeTick;  
  enableInterrupts();
  return tick;
}

static void delayMs(uint16_t delay)
{
	while(delay--)
		for (uint16_t i = 0; i < 2000; i++);
}

/* Encoder Interrupt Handler */
/**
  * @brief External Interrupt PORTA Interrupt routine.
  * @param  None
  * @retval None
  */
static volatile uint16_t encoderLastTick = 0;
static uint8_t encoderChannelAState = 0;
static uint8_t encoderChannelBState = 0;
static uint8_t encoderChannelAPrevState = 0;
#define ENCODER_DEBOUNCE 10
	
INTERRUPT_HANDLER(EXTI_PORTA_IRQHandler, 3)
{
	encoderChannelAState = (GPIOA->IDR & GPIO_PIN_1) >> 1;
	encoderChannelBState = (GPIOA->IDR & GPIO_PIN_2) >> 2;
	uint16_t currentTick = getCurrentTick();
	if (currentTick - encoderLastTick > ENCODER_DEBOUNCE && encoderChannelAPrevState != encoderChannelAState) {
#ifdef DEBUG
	  	static BitStatus channelAState = RESET;
		static BitStatus channelBState = RESET;
	  	channelAState = encoderChannelAState;
		channelBState = encoderChannelBState;
#endif // DEBUG
		if (encoderChannelAState != encoderChannelBState)
			encoderCounter++;
		else
			encoderCounter--;
		encoderChannelAPrevState = encoderChannelAState;
	}
	encoderLastTick = currentTick;
}


/* Encoder Button IRQ handler */
/**
  * @brief External Interrupt PORTC Interrupt routine.
  * @param  None
  * @retval None
  */
#define DEBOUNCE_TIME 10
static uint32_t lastTick = 0;

static uint8_t count = 0;
static void func(void)
{
	count++;
}

INTERRUPT_HANDLER(EXTI_PORTC_IRQHandler, 5)
{
 	static bool buttonState = TRUE;
	if (getCurrentTick() - lastTick > DEBOUNCE_TIME) {
		if (!GPIO_ReadInputPin(GPIOC, BUTTON_PIN) && buttonState) {
			buttonState = FALSE;
			GPIO_WriteReverse(GPIOC, RED_LED_PIN);
			func();
		} else {
			buttonState = TRUE;
		}
	}
	lastTick = getCurrentTick();
}

/* Application Timer Interrupt Handler */
/**
  * @brief Timer1 Update/Overflow/Trigger/Break Interrupt routine.
  * @param  None
  * @retval None
  */
INTERRUPT_HANDLER(TIM1_UPD_OVF_TRG_BRK_IRQHandler, 11)
{
	timeTick++;
	TIM1_ClearITPendingBit(TIM1_IT_UPDATE);
}

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

#ifdef USE_FULL_ASSERT

/**
  * @brief  Reports the name of the source file and the source line number
  *   where the assert_param error has occurred.
  * @param file: pointer to the source file name
  * @param line: assert_param error line source number
  * @retval : None
  */
void assert_failed(u8* file, u32 line)
{
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

  /* Infinite loop */
  while (1)
  {
  }
}
#endif