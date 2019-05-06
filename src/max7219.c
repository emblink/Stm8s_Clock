#include "stm8s.h"
#include "spi.h"
#include "main.h"
#include "max7219.h"

#define NULL 0

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

static uint8_t *dataBuff;
static bool isMax7219Inited = FALSE;
static void max7219FillCommandBuff(Max7219Number max7219Number, Max7219Register reg, uint8_t arg);
static void max7219SendData(const uint8_t dataBuff[], uint16_t size);

static void max7219PushData(void)
{
	GPIOC->ODR &= ~SPI_CS_PIN;
}

static inline void max7219LatchData(void)
{
	GPIOC->ODR |= SPI_CS_PIN;
}

bool max7219Init(uint8_t buff[], uint16_t buffSize)
{
//	if (buff == NULL || buffSize < MAX7219_BUFF_SIZE)
//		return FALSE;
	if (dataBuff == NULL)
        dataBuff = buff;
    max7219FillCommandBuff(MAX7219_NUMBER_COUNT, MAX7219_SHUTDOWN_REG, MAX7219_STATE_ENABLE); // Turn On. Normal Operation
    max7219SendData(dataBuff, MAX7219_COMMAND_BUFF_SIZE);
	max7219FillCommandBuff(MAX7219_NUMBER_COUNT, MAX7219_INTENSITY_REG, MAX7219_INTENSITY_LEVEL_DEFAULT);
	max7219SendData(dataBuff, MAX7219_COMMAND_BUFF_SIZE);
	max7219FillCommandBuff(MAX7219_NUMBER_COUNT, MAX7219_DISPLAY_TEST_REG, MAX7219_TEST_DISABLE); // Display-Test off.
	max7219SendData(dataBuff, MAX7219_COMMAND_BUFF_SIZE);
	max7219FillCommandBuff(MAX7219_NUMBER_COUNT, MAX7219_SCAN_LIMIT_REG, 0x07); // Activate all rows.
	max7219SendData(dataBuff, MAX7219_COMMAND_BUFF_SIZE);
	max7219FillCommandBuff(MAX7219_NUMBER_COUNT, MAX7219_DECODE_MODE_REG, 0x00); // No decode mode.
	max7219SendData(dataBuff, MAX7219_COMMAND_BUFF_SIZE);
    max7219FillCommandBuff(MAX7219_NUMBER_COUNT, MAX7219_SHUTDOWN_REG, MAX7219_STATE_ENABLE); // Turn On. Normal Operation
    max7219SendData(dataBuff, MAX7219_COMMAND_BUFF_SIZE);
	return isMax7219Inited = TRUE;
}

bool max7219SendCommand(Max7219Number max7219Number, Max7219Command cmd, Max7219CommandArgument arg)
{
	if (max7219Number > MAX7219_NUMBER_COUNT || !isMax7219Inited)
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
	if (!isMax7219Inited)
		return;
    max7219Init(NULL, NULL);
	for (Max7219Register reg = MAX7219_ROW_1_REG; reg <= MAX7219_ROW_8_REG; reg++) {
		max7219FillCommandBuff(max7219Number, reg, symbol[reg - 1]);
		max7219SendData(dataBuff, MAX7219_COMMAND_BUFF_SIZE);
	}
}

static void max7219SendData(const uint8_t dataBuff[], uint16_t size)
{
	max7219PushData();
	for (uint16_t i = 0; i < size; i++)
		spiPushByte(dataBuff[i]);
	max7219LatchData();
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


/*

function transform array of columns into array of rows, that can be sent to max7219
static void trans_panel(uint8_t *array, uint8_t *fill_array)
{
	for (uint8_t row = 0; row < 8; row++) {
		uint8_t bit = 1 << row;
		for (int8_t i = 7, bit_indx = 0; i >= 0; i--, bit_indx++) {
			uint8_t data = 0x00;
			data = array[i] & bit; // read bit
			data >>= row; // move to 0 position
			data <<= bit_indx; // move to proper position
			if (!data) { // == 0x00
				uint8_t data_and_mask = ~(1 << bit_indx); // create mask
				fill_array[row] &= (data | data_and_mask); // write 0 in bit
			}
			else
				fill_array[row] |= (data); // write 1 in bit
		}
    }
}


void update_screen(uint8_t *screenbuff)
{
	uint8_t buff_to_send[LED_NUM * 8];
	for (uint8_t n = 0; n < LED_NUM; n++) {
		uint8_t indx = n << 3;
		trans_panel(&screenbuff[indx], &buff_to_send[indx]);
		max7219_send_char_to(n, &buff_to_send[indx]);
	}
}

*/