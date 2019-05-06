#include "main.h"
#include "stm8s.h"
#include "stm8s_exti.h"
#include "stm8s_tim1.h"
#include "stm8s_tim2.h"
#include "stm8s_clk.h"
#include "font.h"
#include "max7219.h"
#include "DS1307.h"
#include "i2c.h"
#include "spi.h"

typedef enum ClockMode {
	CLOCK_MODE_HOURS_MINUTES,
	CLOCK_MODE_MINUTES_SECONDS,
	CLOCK_MODE_SETTINGS,
	CLOCK_MODE_COUNT,
} ClockMode;

typedef enum SettingsMode {
	SETTINGS_MODE_HOURS,
	SETTINGS_MODE_MINUTES,
	SETTINGS_MODE_SECONDS,
	SETTINGS_MODE_APPLY,
	SETTINGS_MODE_DISCARD,
	SETTINGS_MODE_RESET,
	SETTINGS_MODE_COUNT,
} SettingsMode;

typedef union RealTimeClock {
	struct {
		uint8_t hours;
		uint8_t minutes;
		uint8_t seconds;
	} time;
	uint8_t data[sizeof(time)];
} RealTimeClock;

static const uint16_t modeUpdatePeriod[CLOCK_MODE_COUNT] = {
	[CLOCK_MODE_HOURS_MINUTES] = 1000,
	[CLOCK_MODE_MINUTES_SECONDS] = 1000,
	[CLOCK_MODE_SETTINGS] = 250,
};

static void processClockMode(void);
static void processSettingsMode(void);
static void highlightSettingsValue(void);
static uint16_t getCurrentTick(void);
static void updateTime(void);
static void swichClockMode(void);

static ClockMode clockMode = CLOCK_MODE_HOURS_MINUTES;
static SettingsMode settingsMode = SETTINGS_MODE_HOURS;
static volatile bool panelProcess = TRUE;
static volatile bool buttonHoldEvent = FALSE;
static volatile bool settingsHoldEvent = FALSE;
static uint8_t *encoderCounter = NULL;
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
	
    spiInit();
	spiEnable();
	
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
		if (!(timeTick % modeUpdatePeriod[clockMode]) || panelProcess) {
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
		//wfi();
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
		settingsMode = SETTINGS_MODE_HOURS;
		encoderCounter = &rtc.data[settingsMode];
		GPIO_Init(GPIOA, ENCODER_CHANNEL_A_PIN, GPIO_MODE_IN_FL_IT); // A1 // enable encoder Interrupts
		max7219SendSymbol(MAX7219_NUMBER_COUNT, fontGetSpaceArray());
		settingsInited = TRUE;
		panelProcess = FALSE;
	}
	
	highlightSettingsValue();

	if (settingsHoldEvent) {
		switch(settingsMode) {	
		case SETTINGS_MODE_APPLY:
			ds1307_set_hours(rtc.time.hours);
			ds1307_set_minutes(rtc.time.minutes);
			ds1307_set_seconds(rtc.time.seconds);
			break;
		case SETTINGS_MODE_DISCARD:
			updateTime();
			break;
		case SETTINGS_MODE_RESET:
			ds1307_reset();
			break;
		default:
			settingsInited = FALSE;
			break;
		}
	}
	
	if (settingsHoldEvent) {
		GPIO_Init(GPIOA, ENCODER_CHANNEL_A_PIN, GPIO_MODE_IN_FL_NO_IT); // disable encoder Interrupts
		encoderCounter = NULL;
		clockMode = CLOCK_MODE_HOURS_MINUTES;
		panelProcess = TRUE;
        settingsHoldEvent = FALSE;
        settingsInited = FALSE;
	}
}

static void swichClockMode(void)
{
	switch (clockMode) {
	case CLOCK_MODE_HOURS_MINUTES:
		clockMode = CLOCK_MODE_MINUTES_SECONDS;
		panelProcess = TRUE;
		break;
	case CLOCK_MODE_MINUTES_SECONDS:
		clockMode = CLOCK_MODE_HOURS_MINUTES;
		panelProcess = TRUE;
		break;
	case CLOCK_MODE_SETTINGS:
		switch(settingsMode) {
		case SETTINGS_MODE_HOURS:
			settingsMode = SETTINGS_MODE_MINUTES;
			encoderCounter = &rtc.data[settingsMode];
			break;
		case SETTINGS_MODE_MINUTES:
			settingsMode = SETTINGS_MODE_SECONDS;
			encoderCounter = &rtc.data[settingsMode];
			break;
		case SETTINGS_MODE_SECONDS:
			settingsMode = SETTINGS_MODE_APPLY;
			encoderCounter = NULL;
			break;
		case SETTINGS_MODE_APPLY:
			settingsMode = SETTINGS_MODE_DISCARD;
			break;
		case SETTINGS_MODE_DISCARD:
			settingsMode = SETTINGS_MODE_RESET;
			break;
		case SETTINGS_MODE_RESET:
		default:
			settingsMode = SETTINGS_MODE_HOURS;
			encoderCounter = &rtc.data[settingsMode];
			break;
		}
		break;
	default:
		break;
	}
}

static void highlightSettingsValue()
{
	static bool blink = FALSE;
	switch(settingsMode) {
	case SETTINGS_MODE_HOURS:
	case SETTINGS_MODE_MINUTES:
		if (blink) {
			max7219SendSymbol(MAX7219_NUMBER_0, fontGetSpaceArray());
			max7219SendSymbol(MAX7219_NUMBER_1, fontGetSpaceArray());
		} else {
			max7219SendSymbol(MAX7219_NUMBER_0, fontGetNumberArray(rtc.data[settingsMode] / 10));
			max7219SendSymbol(MAX7219_NUMBER_1, fontGetNumberArray(rtc.data[settingsMode] % 10));
		}
		max7219SendSymbol(MAX7219_NUMBER_2, fontGetNumberArray(rtc.data[settingsMode + 1] / 10));
		max7219SendSymbol(MAX7219_NUMBER_3, fontGetNumberArray(rtc.data[settingsMode + 1] % 10));
		break;			
	case SETTINGS_MODE_SECONDS:
		max7219SendSymbol(MAX7219_NUMBER_0, fontGetNumberArray(rtc.time.minutes / 10));
		max7219SendSymbol(MAX7219_NUMBER_1, fontGetNumberArray(rtc.time.minutes % 10));
		if (blink) {
			max7219SendSymbol(MAX7219_NUMBER_2, fontGetSpaceArray());
			max7219SendSymbol(MAX7219_NUMBER_3, fontGetSpaceArray());
		} else {
			max7219SendSymbol(MAX7219_NUMBER_2, fontGetNumberArray(rtc.data[settingsMode] / 10));
			max7219SendSymbol(MAX7219_NUMBER_3, fontGetNumberArray(rtc.data[settingsMode] % 10));
		}
		break;
	case SETTINGS_MODE_APPLY:
		max7219SendSymbol(MAX7219_NUMBER_0, fontGetCharArray('A')); // A
		max7219SendSymbol(MAX7219_NUMBER_1, fontGetCharArray('P')); // P
		max7219SendSymbol(MAX7219_NUMBER_2, fontGetCharArray('L')); // L
		max7219SendSymbol(MAX7219_NUMBER_3, fontGetCharArray('Y')); // Y
		break;
	case SETTINGS_MODE_DISCARD:
		max7219SendSymbol(MAX7219_NUMBER_0, fontGetCharArray('E')); // E
		max7219SendSymbol(MAX7219_NUMBER_1, fontGetCharArray('X')); // X
		max7219SendSymbol(MAX7219_NUMBER_2, fontGetCharArray('I')); // I
		max7219SendSymbol(MAX7219_NUMBER_3, fontGetCharArray('T')); // T
		break;
	case SETTINGS_MODE_RESET:
		max7219SendSymbol(MAX7219_NUMBER_0, fontGetCharArray('R')); // R
		max7219SendSymbol(MAX7219_NUMBER_1, fontGetCharArray('S')); // S
		max7219SendSymbol(MAX7219_NUMBER_2, fontGetCharArray('E')); // E
		max7219SendSymbol(MAX7219_NUMBER_3, fontGetCharArray('T')); // T
		break;
	default:
		break;
	}
	blink ^= TRUE;
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

/* Encoder Interrupt Handler */
/**
  * @brief External Interrupt PORTA Interrupt routine.
  * @param  None
  * @retval None
  */
static volatile uint16_t encoderLastTick = 0;
static uint8_t encoderChannelAState = 0;
static uint8_t encoderChannelBState = 0;
#define ENCODER_DEBOUNCE 10 // add capasitors 
	
INTERRUPT_HANDLER(EXTI_PORTA_IRQHandler, 3)
{
	encoderChannelAState = (GPIOA->IDR & GPIO_PIN_1) >> 1;
	encoderChannelBState = (GPIOA->IDR & GPIO_PIN_2) >> 2;
	static uint16_t counter = 0;
	counter++;
	uint16_t currentTick = getCurrentTick();
	if (currentTick - encoderLastTick > ENCODER_DEBOUNCE) {
		static uint16_t ISR_COUNTER = 0;
		ISR_COUNTER++;
		if (encoderChannelAState != encoderChannelBState) {
			if (encoderCounter) {
				*encoderCounter += 1;
				*encoderCounter %= 60;
			}
		} else {
			if (encoderCounter) {
				if (*encoderCounter > 0) {
					*encoderCounter -= 1;
					*encoderCounter %= 60;
				} else {
					*encoderCounter = 59;
				}
			}
		}
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

INTERRUPT_HANDLER(EXTI_PORTC_IRQHandler, 5)
{
	static bool buttonState = TRUE;
	if (getCurrentTick() - lastTick > DEBOUNCE_TIME) {
		if (!GPIO_ReadInputPin(GPIOC, BUTTON_PIN) && buttonState) {
			buttonState = FALSE;
			TIM2_Cmd(ENABLE);
		} else {
			buttonState = TRUE;
			TIM2_Cmd(DISABLE);
			if (buttonHoldEvent) {
				buttonHoldEvent = FALSE;
			} else {
				swichClockMode();
			}
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
	if (clockMode != CLOCK_MODE_SETTINGS) {
		clockMode = CLOCK_MODE_SETTINGS;
	} else {
		settingsHoldEvent = TRUE;
	}
	buttonHoldEvent = TRUE;
	GPIO_WriteReverse(GPIOC, RED_LED_PIN);
}

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