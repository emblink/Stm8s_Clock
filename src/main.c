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
#include "iwdg.h"
#include "adc.h"

#define SECONDS(sec) (1000U * sec)

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
    SETTINGS_MODE_DAY,
    SETTINGS_MODE_DATE,
    SETTINGS_MODE_MONTH,
    SETTINGS_MODE_YEAR,
	SETTINGS_MODE_APPLY,
	SETTINGS_MODE_DISCARD,
	SETTINGS_MODE_RESET,
    SETTINGS_MODE_BATTERY,
	SETTINGS_MODE_COUNT,
} SettingsMode;

typedef union RealTimeClock {
	struct {
		uint8_t hours;
		uint8_t minutes;
		uint8_t seconds;
        uint8_t day;
        uint8_t date;
        uint8_t month;
        uint8_t year;
	} time;
	uint8_t data[sizeof(time)];
} RealTimeClock;

static const uint16_t modeUpdatePeriod[CLOCK_MODE_COUNT] = {
	[CLOCK_MODE_HOURS_MINUTES] = SECONDS(1),
	[CLOCK_MODE_MINUTES_SECONDS] = SECONDS(1),
	[CLOCK_MODE_SETTINGS] = 250,
};

static void processClockMode(void);
static void processSettingsMode(void);
static void highlightSettingsValue(void);
static bool updateTime(void);
static void swichClockMode(void);
static uint8_t getEncoderTimeDivider(void);
static bool startAdcMeasurment(AdcChannel channel);
static void onAcdMeasurmentCallback(void);
static void processAdcMeasurmetns(void);
static void checkSeasonalClockChange(void);
static void secondsStealer(void);

static const uint16_t brightnessAdjustPeriod = SECONDS(10);
static ClockMode clockMode = CLOCK_MODE_HOURS_MINUTES;
static SettingsMode settingsMode = SETTINGS_MODE_HOURS;
static volatile bool panelProcess = TRUE;
static volatile bool buttonHoldEvent = FALSE;
static volatile bool settingsHoldEvent = FALSE;
static uint8_t *encoderCounter = NULL;
static volatile uint16_t timeTick = 0;
static RealTimeClock rtc = {0};
static uint16_t adcBuffer[ADC_BUFFER_SIZE] = {0};
static bool processAdc = FALSE;
static AdcChannel adcCurrentChannel = ADC_PHOTO_CHANNEL;
static uint32_t batteryVoltage = 0;
static uint8_t lastStealedSecondHour = 0;

int main( void )
{
	/* Configure system clock */
	CLK_HSICmd(ENABLE);
	while (!CLK->ICKR & (1 << 1)); // wait untill clock became stable
	CLK_SYSCLKConfig(CLK_PRESCALER_HSIDIV1); // 16MHz SYS
	CLK_SYSCLKConfig(CLK_PRESCALER_CPUDIV1); // 16MHz CPU
    
    for (uint32_t i = 0; i < U16_MAX; i++); // startup delay, in order to get stable supply voltage
    
    /* Enable clocks for peripherals */
    CLK_PeripheralClockConfig(CLK_PERIPHERAL_I2C, ENABLE);
	CLK_PeripheralClockConfig(CLK_PERIPHERAL_SPI, ENABLE);
	CLK_PeripheralClockConfig(CLK_PERIPHERAL_TIMER1, ENABLE);
    CLK_PeripheralClockConfig(CLK_PERIPHERAL_TIMER2, ENABLE);
	CLK_PeripheralClockConfig(CLK_PERIPHERAL_ADC, ENABLE);
    
    /* Init GPIO */
    GPIO_Init(BATT_MEASURE_EN_PORT, BATT_MEASURE_EN_PIN, GPIO_MODE_OUT_PP_LOW_SLOW);
    GPIO_Init(DEBUG_PORT, DEBUG_PIN, GPIO_MODE_OUT_PP_LOW_SLOW);
    
    /* Init I2c */
	i2cInit();
    
    /* Check Hard Reset conditon */
    GPIO_Init(ENCODER_BUTTON_PORT, ENCODER_BUTTON_PIN, GPIO_MODE_IN_PU_NO_IT);
    if (!GPIO_ReadInputPin(ENCODER_BUTTON_PORT, ENCODER_BUTTON_PIN))
        ds1307_reset();
    
    /* Init Encoder */
    GPIO_Init(ENCODER_BUTTON_PORT, ENCODER_BUTTON_PIN, GPIO_MODE_IN_PU_IT);
    GPIO_Init(ENCODER_CHANNEL_A_PORT, ENCODER_CHANNEL_A_PIN, GPIO_MODE_IN_PU_NO_IT);
    GPIO_Init(ENCODER_CHANNEL_B_PORT, ENCODER_CHANNEL_B_PIN, GPIO_MODE_IN_PU_NO_IT);
    EXTI_SetExtIntSensitivity(EXTI_PORT_GPIOD, EXTI_SENSITIVITY_FALL_ONLY);
    
    /* Init SPI */
	GPIO_Init(GPIOC, SPI_CS_PIN, GPIO_MODE_OUT_PP_HIGH_SLOW);
	GPIO_Init(GPIOC, SPI_MOSI_PIN, GPIO_MODE_OUT_PP_HIGH_FAST);
	GPIO_Init(GPIOC, SPI_SCK_PIN, GPIO_MODE_OUT_PP_HIGH_FAST);
    spiInit();
	spiEnable();
	
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

    /* Independent WatchDog Timer Innit */
    iwdgInit();
    
	/* Max7219 Init */
	max7219Init();
    
    /* ADC init */
	adcInit();

	enableInterrupts();
	
	/* Start application timer */
	TIM1_Cmd(ENABLE);
	
	while(1)
	{
        static uint16_t lastModeUpdateTick = 0;
        static uint16_t lastAdcPhotoMeasurementTick = 0;
		if (timeTick - lastModeUpdateTick >= modeUpdatePeriod[clockMode]  || panelProcess) {
			switch (clockMode) {
			case CLOCK_MODE_HOURS_MINUTES:
			case CLOCK_MODE_MINUTES_SECONDS:
                if (updateTime() == FALSE) {
                    i2cDeInit();
                    i2cInit();
                    panelProcess = TRUE;
                    continue;
                }
                checkSeasonalClockChange();
                processClockMode();
                secondsStealer();
                if (timeTick - lastAdcPhotoMeasurementTick >= brightnessAdjustPeriod) {
                    startAdcMeasurment(ADC_PHOTO_CHANNEL);
                    lastAdcPhotoMeasurementTick = timeTick;
                }
                panelProcess = FALSE;
				break;
			case CLOCK_MODE_SETTINGS:
				processSettingsMode();
				break;
			default:
				break;
			}
            lastModeUpdateTick = timeTick;
		}

        // TODO: Use timer 2 or 4 as SysTick, use timer 1 as TRGO for ADC, configure for periodic 1 minute measurments     
        if (processAdc) {
            processAdcMeasurmetns();
            processAdc = FALSE;
        }
        iwdgFeed();
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
}

static void processSettingsMode(void) 
{	
	static bool settingsInited = FALSE;
	
	if (!settingsInited) {
		settingsMode = SETTINGS_MODE_HOURS;
		encoderCounter = &rtc.data[settingsMode];
        GPIO_Init(ENCODER_CHANNEL_A_PORT, ENCODER_CHANNEL_A_PIN, GPIO_MODE_IN_PU_IT); //enable encoder Interrupts
        EXTI_SetExtIntSensitivity(EXTI_PORT_GPIOC, EXTI_SENSITIVITY_FALL_ONLY);
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
            ds1307_set_day(rtc.time.day);
            ds1307_set_date(rtc.time.date);
			ds1307_set_month(rtc.time.month);
			ds1307_set_year(rtc.time.year);
			break;
		case SETTINGS_MODE_DISCARD:
			break;
		case SETTINGS_MODE_RESET:
			ds1307_reset();
			break;
        case SETTINGS_MODE_BATTERY:
            break;
		default:
			settingsInited = FALSE;
			break;
		}
	}
	
	if (settingsHoldEvent) {
        GPIO_Init(ENCODER_CHANNEL_A_PORT, ENCODER_CHANNEL_A_PIN, GPIO_MODE_IN_PU_NO_IT); //disable encoder Interrupts
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
        settingsMode++;
        settingsMode %= SETTINGS_MODE_COUNT;
        encoderCounter = settingsMode < SETTINGS_MODE_APPLY ? &rtc.data[settingsMode] : NULL;
		switch(settingsMode) {
        case SETTINGS_MODE_BATTERY:
            startAdcMeasurment(ADC_BATT_CHANNEL);
			break;
		default:
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
    case SETTINGS_MODE_DAY:
        max7219SendSymbol(MAX7219_NUMBER_0, fontGetCharArray('D'));
		max7219SendSymbol(MAX7219_NUMBER_1, fontGetCharArray('Y'));
        if (blink) {
			max7219SendSymbol(MAX7219_NUMBER_2, fontGetSpaceArray());
			max7219SendSymbol(MAX7219_NUMBER_3, fontGetSpaceArray());
		} else {
            max7219SendSymbol(MAX7219_NUMBER_2, fontGetNumberArray(rtc.data[settingsMode] / 10));
            max7219SendSymbol(MAX7219_NUMBER_3, fontGetNumberArray(rtc.data[settingsMode] % 10));
		}
        break;
    case SETTINGS_MODE_DATE:
        max7219SendSymbol(MAX7219_NUMBER_0, fontGetCharArray('D'));
		max7219SendSymbol(MAX7219_NUMBER_1, fontGetCharArray('T'));
        if (blink) {
			max7219SendSymbol(MAX7219_NUMBER_2, fontGetSpaceArray());
			max7219SendSymbol(MAX7219_NUMBER_3, fontGetSpaceArray());
		} else {
            max7219SendSymbol(MAX7219_NUMBER_2, fontGetNumberArray(rtc.data[settingsMode] / 10));
            max7219SendSymbol(MAX7219_NUMBER_3, fontGetNumberArray(rtc.data[settingsMode] % 10));
		}
        break;
    case SETTINGS_MODE_MONTH:
        max7219SendSymbol(MAX7219_NUMBER_0, fontGetCharArray('M'));
		max7219SendSymbol(MAX7219_NUMBER_1, fontGetCharArray('N'));
        if (blink) {
			max7219SendSymbol(MAX7219_NUMBER_2, fontGetSpaceArray());
			max7219SendSymbol(MAX7219_NUMBER_3, fontGetSpaceArray());
		} else {
            max7219SendSymbol(MAX7219_NUMBER_2, fontGetNumberArray(rtc.data[settingsMode] / 10));
            max7219SendSymbol(MAX7219_NUMBER_3, fontGetNumberArray(rtc.data[settingsMode] % 10));
		}
        break;
    case SETTINGS_MODE_YEAR:
        max7219SendSymbol(MAX7219_NUMBER_0, fontGetCharArray('Y'));
		max7219SendSymbol(MAX7219_NUMBER_1, fontGetCharArray('E'));
        if (blink) {
			max7219SendSymbol(MAX7219_NUMBER_2, fontGetSpaceArray());
			max7219SendSymbol(MAX7219_NUMBER_3, fontGetSpaceArray());
		} else {
            max7219SendSymbol(MAX7219_NUMBER_2, fontGetNumberArray(rtc.data[settingsMode] / 10));
            max7219SendSymbol(MAX7219_NUMBER_3, fontGetNumberArray(rtc.data[settingsMode] % 10));   
		}
        break;
	case SETTINGS_MODE_APPLY:
		max7219SendSymbol(MAX7219_NUMBER_0, fontGetCharArray('A'));
		max7219SendSymbol(MAX7219_NUMBER_1, fontGetCharArray('P'));
		max7219SendSymbol(MAX7219_NUMBER_2, fontGetCharArray('L'));
		max7219SendSymbol(MAX7219_NUMBER_3, fontGetCharArray('Y'));
		break;
	case SETTINGS_MODE_DISCARD:
		max7219SendSymbol(MAX7219_NUMBER_0, fontGetCharArray('E'));
		max7219SendSymbol(MAX7219_NUMBER_1, fontGetCharArray('X'));
		max7219SendSymbol(MAX7219_NUMBER_2, fontGetCharArray('I'));
		max7219SendSymbol(MAX7219_NUMBER_3, fontGetCharArray('T'));
		break;
	case SETTINGS_MODE_RESET:
		max7219SendSymbol(MAX7219_NUMBER_0, fontGetCharArray('R'));
		max7219SendSymbol(MAX7219_NUMBER_1, fontGetCharArray('S'));
		max7219SendSymbol(MAX7219_NUMBER_2, fontGetCharArray('E'));
		max7219SendSymbol(MAX7219_NUMBER_3, fontGetCharArray('T'));
		break;
    case SETTINGS_MODE_BATTERY:
        max7219SendSymbol(MAX7219_NUMBER_0, fontGetNumberArray(batteryVoltage / 1000 - '0'));
		max7219SendSymbol(MAX7219_NUMBER_1, fontGetNumberArray(batteryVoltage /100 % 10 - '0'));
		max7219SendSymbol(MAX7219_NUMBER_2, fontGetNumberArray(batteryVoltage / 10 % 10 - '0'));
		max7219SendSymbol(MAX7219_NUMBER_3, fontGetNumberArray(batteryVoltage % 10 - '0'));
        break;
	default:
		break;
	}
	blink ^= TRUE;
}

static bool updateTime(void)
{
    bool status = FALSE;
    status |= !ds1307_get_hours(&rtc.time.hours);
    status |= !ds1307_get_minutes(&rtc.time.minutes);
    status |= !ds1307_get_seconds(&rtc.time.seconds);
    status |= !ds1307_get_day(&rtc.time.day);
    status |= !ds1307_get_date(&rtc.time.date);
    status |= !ds1307_get_month(&rtc.time.month);
    status |= !ds1307_get_year(&rtc.time.year);
    return status == FALSE ? TRUE : FALSE;
}

static void checkSeasonalClockChange(void)
{   
    static bool applySwitch = TRUE;
    /* last sunday of march */
    if (rtc.time.month == 3 && rtc.time.date > 23 && rtc.time.day == 7 
        && rtc.time.hours == 3) {
        ds1307_set_hours(rtc.time.hours + 1);
        applySwitch = TRUE;
    }
    /* last sunday of october */
    if (rtc.time.month == 10 && rtc.time.date > 23 && rtc.time.day == 7 
        && rtc.time.hours == 4) {
        if (applySwitch) {
            ds1307_set_hours(rtc.time.hours - 1);
            applySwitch = FALSE;
        }
    }
}

static void secondsStealer(void)
{
    // subtract a second every 4 hours, in order to get proper time
    if (rtc.time.hours % 4 == 0 && rtc.time.minutes == 0 && rtc.time.seconds == 2) {
        if (lastStealedSecondHour != rtc.time.hours) {
            ds1307_set_seconds(rtc.time.seconds - 1);
            lastStealedSecondHour = rtc.time.hours;
        }
    }
}

static uint8_t getEncoderTimeDivider(void)
{
    switch(settingsMode) {
    case SETTINGS_MODE_HOURS:
        return 24;
    case SETTINGS_MODE_MINUTES:
    case SETTINGS_MODE_SECONDS:
        return 60;
    case SETTINGS_MODE_DAY:
        return 8;
    case SETTINGS_MODE_DATE:
        return 32;
    case SETTINGS_MODE_MONTH:
        return 13;
    case SETTINGS_MODE_YEAR:
        return 100;
    default:
        return 0;
    }    
}

static bool startAdcMeasurment(AdcChannel channel)
{
    adcCurrentChannel = channel;
    adcStop();
    switch(channel) {
    case ADC_PHOTO_CHANNEL:
        return adcStartMesurment(ADC_PHOTO_CHANNEL, onAcdMeasurmentCallback);
    case ADC_BATT_CHANNEL:
        GPIO_WriteHigh(BATT_MEASURE_EN_PORT, BATT_MEASURE_EN_PIN);
        return adcStartMesurment(ADC_BATT_CHANNEL, onAcdMeasurmentCallback);
    default:
        return FALSE;
    }
}

static void processAdcMeasurmetns(void)
{
    uint32_t value = 0;
    for (uint8_t i = 0; i < ADC_BUFFER_SIZE; i++)
        value += adcBuffer[i];
    value /= ADC_BUFFER_SIZE;
    
    switch(adcCurrentChannel) {
    case ADC_PHOTO_CHANNEL:
        if (value > 700)
            max7219SendCommand(MAX7219_NUMBER_COUNT, MAX7219_SET_INTENSITY_LEVEL, MAX7219_INTENSITY_LEVEL_0);
        else if (value > 300)
            max7219SendCommand(MAX7219_NUMBER_COUNT, MAX7219_SET_INTENSITY_LEVEL, MAX7219_INTENSITY_LEVEL_1);
        else
            max7219SendCommand(MAX7219_NUMBER_COUNT, MAX7219_SET_INTENSITY_LEVEL, MAX7219_INTENSITY_LEVEL_2);
        break;
    case ADC_BATT_CHANNEL: {
        GPIO_WriteLow(BATT_MEASURE_EN_PORT, BATT_MEASURE_EN_PIN);
        batteryVoltage = (3222 * value) / 1000; // oneAdcPoint = 3.3v / 1024 == 0,003222 V == 3,222 mV / 1000
    } break;
    default:
        break;
    }
}

static void onAcdMeasurmentCallback(void)
{
    adcGetBufferedData(adcBuffer);
    adcStop();
    processAdc = TRUE;
}

/* Encoder Button IRQ handler */
#define DEBOUNCE_TIME 10

INTERRUPT_HANDLER(EXTI_PORTD_IRQHandler, 6)
{
    static uint16_t lastTick = 0;
	if (timeTick - lastTick > DEBOUNCE_TIME) {
        EXTI_Sensitivity_TypeDef sence = EXTI_GetExtIntSensitivity(EXTI_PORT_GPIOD);
		if (sence == EXTI_SENSITIVITY_FALL_ONLY) {
            EXTI_SetExtIntSensitivity(EXTI_PORT_GPIOD, EXTI_SENSITIVITY_RISE_ONLY);
			TIM2_Cmd(ENABLE);
		} else {
            EXTI_SetExtIntSensitivity(EXTI_PORT_GPIOD, EXTI_SENSITIVITY_FALL_ONLY);
			TIM2_Cmd(DISABLE);
            TIM2_SetCounter(0);
			if (buttonHoldEvent) {
				buttonHoldEvent = FALSE;
			} else {
				swichClockMode();
			}
		}
	}
	lastTick = timeTick;
}

/* Encoder Interrupt Handler */
#define ENCODER_DEBOUNCE 1

INTERRUPT_HANDLER(EXTI_PORTC_IRQHandler, 5)
{
    static uint16_t encoderLastTick = 0;
	if (timeTick - encoderLastTick > ENCODER_DEBOUNCE) {
        EXTI_Sensitivity_TypeDef sence = EXTI_GetExtIntSensitivity(EXTI_PORT_GPIOC);
        if (sence == EXTI_SENSITIVITY_RISE_ONLY) {
            EXTI_SetExtIntSensitivity(EXTI_PORT_GPIOC, EXTI_SENSITIVITY_FALL_ONLY);
            encoderLastTick = timeTick;
            return;
        } else {
            EXTI_SetExtIntSensitivity(EXTI_PORT_GPIOC, EXTI_SENSITIVITY_RISE_ONLY);
        }
        if (encoderCounter == NULL)
            return;
        bool channelB = (bool) (GPIO_ReadInputPin(ENCODER_CHANNEL_B_PORT, ENCODER_CHANNEL_B_PIN) == SET);
        GPIO_WriteHigh(DEBUG_PORT, DEBUG_PIN);
        GPIO_WriteLow(DEBUG_PORT, DEBUG_PIN);
        if (channelB) {
            *encoderCounter += 1;
            *encoderCounter %= getEncoderTimeDivider();
        } else {
            if (*encoderCounter > 0) {
                *encoderCounter -= 1;
                *encoderCounter %= getEncoderTimeDivider();
            } else {
                *encoderCounter = getEncoderTimeDivider() - 1;
            }
        }
	}
	encoderLastTick = timeTick;
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