#include "stm8s.h"
#include "spi.h"
#include "max7219.h"

typedef enum Max7219Register {
    MAX7219_NOOP_REG = 0x00,
    MAX7219_ROW_1_REG = 0x01,
    MAX7219_ROW_2_REG = 0x02,
    MAX7219_ROW_3_REG = 0x03,
    MAX7219_ROW_4_REG = 0x04,
    MAX7219_ROW_5_REG = 0x05,
    MAX7219_ROW_6_REG = 0x06,
    MAX7219_ROW_7_REG = 0x07,
    MAX7219_ROW_8_REG = 0x08,
    MAX7219_DECODE_MODE_REG = 0x9,
    MAX7219_INTENSITY_REG = 0xA,
    MAX7219_SCAN_LIMIT_REG = 0xB,
    MAX7219_SHUTDOWN_REG = 0xC,
    MAX7219_DISPLAY_TEST_REG = 0xF
} Max7219Register;

static uint8_t dataBuff[MAX7219_BUFF_SIZE];
static void max7219FillCommandBuff(Max7219Number max7219Number, Max7219Register reg, uint8_t arg);
static void max7219SendSettings(void);
static void max7219SendData(const uint8_t dataBuff[], uint16_t size);

void max7219Init(void)
{
    max7219SendSettings();
    max7219FillCommandBuff(MAX7219_NUMBER_COUNT, MAX7219_INTENSITY_REG, MAX7219_INTENSITY_LEVEL_DEFAULT);
	max7219SendData(dataBuff, MAX7219_COMMAND_BUFF_SIZE);
}

bool max7219SendCommand(Max7219Number max7219Number, Max7219Command cmd, Max7219CommandArgument arg)
{
	if (max7219Number > MAX7219_NUMBER_COUNT)
		return FALSE;
	
	Max7219Register reg;
	switch(cmd) {
	case MAX7219_SET_STATE:
		reg = MAX7219_SHUTDOWN_REG;
		if (arg > MAX7219_STATE_ENABLE)
			return FALSE;
		break;
	case MAX7219_SET_TEST_MODE:
		reg = MAX7219_DISPLAY_TEST_REG;
		if (arg > MAX7219_TEST_ENABLE)
		break;
	case MAX7219_SET_INTENSITY_LEVEL:
		reg = MAX7219_INTENSITY_REG;
		if (arg > MAX7219_INTENSITY_LEVEL_15)
			return FALSE;
		break;
	default:
		return FALSE;
	}
	max7219FillCommandBuff(max7219Number, reg, arg);
	max7219SendData(dataBuff, MAX7219_COMMAND_BUFF_SIZE);
	return TRUE;
}

void max7219SendSymbol(Max7219Number max7219Number, const uint8_t symbol[FONT_SYMBOL_SIZE_IN_BYTES])
{
    max7219SendSettings();
	for (Max7219Register reg = MAX7219_ROW_1_REG; reg <= MAX7219_ROW_8_REG; reg++) {
		max7219FillCommandBuff(max7219Number, reg, symbol[reg - 1]);
		max7219SendData(dataBuff, MAX7219_COMMAND_BUFF_SIZE);
	}
}

static void max7219SendData(const uint8_t dataBuff[], uint16_t size)
{
    spiSendData(dataBuff, size);
}

static void max7219SendSettings(void)
{
    max7219FillCommandBuff(MAX7219_NUMBER_COUNT, MAX7219_SHUTDOWN_REG, MAX7219_STATE_ENABLE); // Turn On. Normal Operation
    max7219SendData(dataBuff, MAX7219_COMMAND_BUFF_SIZE);
	max7219FillCommandBuff(MAX7219_NUMBER_COUNT, MAX7219_DISPLAY_TEST_REG, MAX7219_TEST_DISABLE); // Display-Test off.
	max7219SendData(dataBuff, MAX7219_COMMAND_BUFF_SIZE);
	max7219FillCommandBuff(MAX7219_NUMBER_COUNT, MAX7219_SCAN_LIMIT_REG, 0x07); // Activate all rows.
	max7219SendData(dataBuff, MAX7219_COMMAND_BUFF_SIZE);
	max7219FillCommandBuff(MAX7219_NUMBER_COUNT, MAX7219_DECODE_MODE_REG, 0x00); // No decode mode.
	max7219SendData(dataBuff, MAX7219_COMMAND_BUFF_SIZE); 
}

static void max7219FillCommandBuff(Max7219Number max7219Number, Max7219Register reg, uint8_t arg)
{
	bool isCommandCommon = FALSE;
	if (max7219Number == MAX7219_NUMBER_COUNT)
		isCommandCommon = TRUE;

	for (uint8_t i = MAX7219_NUMBER_0, idx = i; i < MAX7219_NUMBER_COUNT; i++) {
		if (isCommandCommon) {
			dataBuff[idx++] = reg;
			dataBuff[idx++] = arg;
		} else {
			if (i == max7219Number) {
				dataBuff[idx++] = reg;
				dataBuff[idx++] = arg;
			} else {
				dataBuff[idx++] = MAX7219_NOOP_REG; // No Operation
				dataBuff[idx++] = 0x00;
			}
		}
	}
}