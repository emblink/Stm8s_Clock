#include "main.h"
#include "stm8s.h"
#include "stm8s_gpio.h"
#include "stm8s_exti.h"
#include "stm8s_tim1.h"
#include "stm8s_tim2.h"
#include "stm8s_clk.h"
#include "stm8s_spi.h"
#include "stm8s_i2c.h"
#include "font.h"
#include "max7219.h"
#include "DS1307.h"
#include "i2c.h"

#define DEBUG
#define SETTINGS_MODE_DISCARD_HOLD_PERIOD_MS 500
#define SETTINGS_MODE_ENTER_APPLY_HOLD_PERIOD_MS 2000
#define SETTINGS_MODE_RESET_TO_DEFAULTS_MS 5000

typedef enum ClockMode {
	CLOCK_MODE_HOURS_MINUTES = 0,
	CLOCK_MODE_MINUTES_SECONDS = 1,
	CLOCK_MODE_SETTINGS,
	CLOCK_MODE_COUNT,
} ClockMode;

typedef enum SettingsParam {
	SETTINGS_PARAM_DISCARD,
	SETTINGS_PARAM_RESET_TO_DEFAULTS,
	SETTINGS_PARAM_APPLY,
	SETTINGS_PARAM_TOGGLE_BRIGHTNESS,
	SETTINGS_PARAM_COUNT,
} SettingsParam;

typedef enum SettingsChangeValue{
	SETTINGS_CHANGE_HOURS = 0,
	SETTINGS_CHANGE_MINUTES = 1,
	SETTINGS_CHANGE_SECONDS = 2,
	SETTINGS_CHANGE_VALUE_COUNT,	
} SettingsChangeValue;

typedef union RealTimeClock {
	struct {
		uint8_t hours;
		uint8_t minutes;
		uint8_t seconds;
	} time;
	uint8_t data[sizeof(time)];
} RealTimeClock;

static const uint16_t clockModeUpdatePeriod[CLOCK_MODE_COUNT] = {
	[CLOCK_MODE_HOURS_MINUTES] = 1000,
	[CLOCK_MODE_MINUTES_SECONDS] = 1000,
	[CLOCK_MODE_SETTINGS] = 100,
};

static void processClockMode(void);
static void processSettingsMode(void);
static void highlightSettingsValue(void);
static uint16_t getCurrentTick(void);
static void updateTime(void);
static void delayMs(uint16_t delay);
static void swichClockMode(void);

static ClockMode clockMode = CLOCK_MODE_HOURS_MINUTES;
static volatile bool panelProcess = TRUE;
static volatile bool brightnessProcess = FALSE;
static uint8_t settingsChangeValue = SETTINGS_CHANGE_VALUE_COUNT;
static SettingsParam settingsParam = SETTINGS_PARAM_COUNT;
static volatile uint16_t timeTick = 0;
static RealTimeClock rtc = {0};



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
	GPIO_Init(GPIOA, ENCODER_CHANNEL_A_PIN, GPIO_MODE_IN_FL_NO_IT); // A1
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
	TIM1_PrescalerConfig(15, TIM1_PSCRELOADMODE_IMMEDIATE); // 15 + 1
	TIM1_SetAutoreload(1000); // frequency = 16 000 000 / 16 / 1000 = 1000 Hz
	TIM1_ITConfig(TIM1_IT_UPDATE, ENABLE);
	TIM1_CounterModeConfig(TIM1_COUNTERMODE_UP);
	
	TIM2_DeInit();
	TIM2_UpdateRequestConfig(TIM2_UPDATESOURCE_REGULAR);
	TIM2_PrescalerConfig(TIM2_PRESCALER_1024, TIM2_PSCRELOADMODE_IMMEDIATE); // frequency = 16M / 1024 / 31250 = 0.5 Hz
	TIM2_SetAutoreload(31250);
	TIM2_SelectOnePulseMode(TIM2_OPMODE_SINGLE);
	TIM2_ITConfig(TIM2_IT_UPDATE, ENABLE);

	/* Max7219 Init */
	static uint8_t max7219Buff[MAX7219_BUFF_SIZE];
	max7219Init(max7219Buff, sizeof(max7219Buff));
	
	enableInterrupts();
	
	/* Start application timer */
	TIM1_Cmd(ENABLE);
	
	while(1)
	{
		if (!(timeTick % clockModeUpdatePeriod[clockMode]) || panelProcess) {
			switch (clockMode) {
			case CLOCK_MODE_HOURS_MINUTES:
			case CLOCK_MODE_MINUTES_SECONDS:
				updateTime();
				processClockMode();
				break;
			case CLOCK_MODE_SETTINGS:
				processSettingsMode();
				break;
			default:
				break;
			}
		}
		wfi();
	}
}

static void processClockMode(void)
{
	static bool blink = FALSE;
	max7219SendSymbol(MAX7219_NUMBER_0, fontGetNumberArray(rtc.data[clockMode] / 10));
	if (blink) {
		max7219SendSymbol(MAX7219_NUMBER_1, fontAddDots(fontGetNumberArray(rtc.data[clockMode] % 10), FONT_SYMBOL_RIGHT_SHIFT));
		max7219SendSymbol(MAX7219_NUMBER_2, fontAddDots(fontGetNumberArrayShifted(rtc.data[clockMode + 1] / 10, FONT_SYMBOL_RIGHT_SHIFT, 2), FONT_SYMBOL_LEFT_SHIFT));
	} else {
		max7219SendSymbol(MAX7219_NUMBER_1, fontGetNumberArray(rtc.data[clockMode] % 10));
		max7219SendSymbol(MAX7219_NUMBER_2, fontGetNumberArrayShifted(rtc.data[clockMode + 1] / 10, FONT_SYMBOL_RIGHT_SHIFT, 2));
	}
	max7219SendSymbol(MAX7219_NUMBER_3, fontGetNumberArrayShifted(rtc.data[clockMode + 1] % 10, FONT_SYMBOL_RIGHT_SHIFT, 2));
	blink ^= TRUE;
	panelProcess = FALSE;
}

static void processSettingsMode(void) {
	
	static bool settingsInited = FALSE;
	
	if (!settingsInited) {
		GPIO_Init(GPIOA, ENCODER_CHANNEL_A_PIN, GPIO_MODE_IN_FL_IT); // A1 // enable encoder Interrupts
		settingsChangeValue = SETTINGS_CHANGE_HOURS;
		max7219SendSymbol(MAX7219_NUMBER_COUNT, fontGetSpaceArray());
		settingsInited = TRUE;
	}
	
	switch(settingsChangeValue) {
	case SETTINGS_CHANGE_HOURS:
		max7219SendSymbol(MAX7219_NUMBER_0, fontGetNumberArray(rtc.data[SETTINGS_CHANGE_HOURS] / 10));
		max7219SendSymbol(MAX7219_NUMBER_1, fontGetNumberArray(rtc.data[SETTINGS_CHANGE_HOURS] % 10));
		max7219SendSymbol(MAX7219_NUMBER_2, fontGetNumberArray(rtc.data[SETTINGS_CHANGE_MINUTES] / 10));
		max7219SendSymbol(MAX7219_NUMBER_3, fontGetNumberArray(rtc.data[SETTINGS_CHANGE_MINUTES] % 10));
		break;
	case SETTINGS_CHANGE_MINUTES:
	case SETTINGS_CHANGE_SECONDS:
		max7219SendSymbol(MAX7219_NUMBER_0, fontGetNumberArray(rtc.data[SETTINGS_CHANGE_MINUTES] / 10));
		max7219SendSymbol(MAX7219_NUMBER_1, fontGetNumberArray(rtc.data[SETTINGS_CHANGE_MINUTES] % 10));
		max7219SendSymbol(MAX7219_NUMBER_2, fontGetNumberArray(rtc.data[SETTINGS_CHANGE_SECONDS] / 10));
		max7219SendSymbol(MAX7219_NUMBER_3, fontGetNumberArray(rtc.data[SETTINGS_CHANGE_SECONDS] % 10));
		break;
	default:
		settingsInited = FALSE;
		settingsParam = SETTINGS_PARAM_COUNT;
		break;
	}
	highlightSettingsValue();
	
	switch (settingsParam) {
	case SETTINGS_PARAM_DISCARD:
		updateTime();
		settingsInited = FALSE;
		break;
	case SETTINGS_PARAM_RESET_TO_DEFAULTS:
		ds1307_reset();
		settingsInited = FALSE;
		break;
	case SETTINGS_PARAM_APPLY:
		ds1307_set_hours(rtc.time.hours);
		ds1307_set_minutes(rtc.time.minutes);
		ds1307_set_seconds(rtc.time.seconds);
		settingsInited = FALSE;
		break;
	default:
		break;
	}
	
	if (!settingsInited) {
		GPIO_Init(GPIOA, ENCODER_CHANNEL_A_PIN, GPIO_MODE_IN_FL_NO_IT); // disable encoder Interrupts
		settingsChangeValue = SETTINGS_CHANGE_HOURS;
		clockMode = CLOCK_MODE_HOURS_MINUTES;
		panelProcess = TRUE;
		return;
	}
	panelProcess = FALSE;
}

static void swichClockMode(void)
{
	switch (clockMode) {
	case CLOCK_MODE_HOURS_MINUTES:
		clockMode = CLOCK_MODE_MINUTES_SECONDS;
		break;
	case CLOCK_MODE_MINUTES_SECONDS:
		clockMode = CLOCK_MODE_HOURS_MINUTES;
		break;
	case CLOCK_MODE_SETTINGS:
		settingsChangeValue++;
		settingsChangeValue %= SETTINGS_CHANGE_VALUE_COUNT;
		break;
	default:
		break;
	}
	panelProcess = TRUE;
}

static void highlightSettingsValue(void)
{
	static uint8_t brightness = 0;
	static int8_t increment = 1;
	brightness += increment;
	switch(settingsChangeValue) {
	case SETTINGS_CHANGE_HOURS:
		max7219SendCommand(MAX7219_NUMBER_0, MAX7219_SET_INTENSITY_LEVEL, (Max7219CommandArgument) brightness);
		max7219SendCommand(MAX7219_NUMBER_1, MAX7219_SET_INTENSITY_LEVEL, (Max7219CommandArgument) brightness);
		break;
	case SETTINGS_CHANGE_MINUTES:
	case SETTINGS_CHANGE_SECONDS:
		max7219SendCommand(MAX7219_NUMBER_2, MAX7219_SET_INTENSITY_LEVEL, (Max7219CommandArgument) brightness);
		max7219SendCommand(MAX7219_NUMBER_3, MAX7219_SET_INTENSITY_LEVEL, (Max7219CommandArgument) brightness);
		break;
	default:
		return;
	}
	if (brightness == MAX7219_INTENSITY_LEVEL_0 || brightness == MAX7219_INTENSITY_LEVEL_10)
		increment = -increment;
}

static void updateTime(void)
{
	rtc.time.hours = ds1307_get_hours();
	rtc.time.minutes = ds1307_get_minutes();
	rtc.time.seconds = ds1307_get_seconds();
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
			rtc.data[settingsChangeValue]++;
		else
			rtc.data[settingsChangeValue]++;
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
static uint16_t lastTick = 0;

#ifdef DEBUG
static volatile uint8_t interruptEnterCount = 0;
static volatile uint8_t pressed = 0;
static volatile uint8_t released = 0;
#endif // DEBUG

INTERRUPT_HANDLER(EXTI_PORTC_IRQHandler, 5)
{
	#ifdef DEBUG
	interruptEnterCount++;
	#endif // DEBUG
	static bool buttonState = TRUE;
	if (getCurrentTick() - lastTick > DEBOUNCE_TIME) {
		if (!GPIO_ReadInputPin(GPIOC, BUTTON_PIN) && buttonState) {
			buttonState = FALSE;
			#ifdef DEBUG
			pressed++;
			#endif // DEBUG
			TIM2_Cmd(ENABLE);
		} else {
			buttonState = TRUE;
			#ifdef DEBUG
			released++;
			#endif // DEBUG
			TIM2_Cmd(DISABLE);
			swichClockMode();
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
	TIM1_ClearITPendingBit(TIM1_IT_UPDATE);
	timeTick++;
}

/**
  * @brief Timer2 Update/Overflow/Break Interrupt routine.
  * @param  None
  * @retval None
  */
INTERRUPT_HANDLER(TIM2_UPD_OVF_BRK_IRQHandler, 13)
{
	TIM2_ClearITPendingBit(TIM2_IT_UPDATE);
	switch(clockMode) {
	case CLOCK_MODE_HOURS_MINUTES:
	case CLOCK_MODE_MINUTES_SECONDS:
		clockMode = CLOCK_MODE_SETTINGS;
		break;
	case CLOCK_MODE_SETTINGS:
		settingsParam = SETTINGS_PARAM_DISCARD;
		break;
	default:
		break;
	}
	GPIO_WriteReverse(GPIOC, RED_LED_PIN);
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