#include "stm8s.h"
#include "font.h"

#define ASCII_OFFSET 48

static const uint8_t fontBigNumberData[][6] = {
	{0xE3, 0xFE, 0x95, 0xD4, 0xFE, 0xE7}, // '0'
	{0x08, 0x88, 0xEF, 0xEF, 0x08, 0x08}, // '1'
	{0x4C, 0x6E, 0x2A, 0x29, 0xE9, 0xC8}, // '2'
	/*
	{0xE3, 0xFE, 0x95, 0xD4, 0xFE, 0xE7}, // '2'
	{0xE3, 0xFE, 0x95, 0xD4, 0xFE, 0xE7}, // '3'
	{0xE3, 0xFE, 0x95, 0xD4, 0xFE, 0xE7}, // '4'
	{0xE3, 0xFE, 0x95, 0xD4, 0xFE, 0xE7}, // '5'
	{0xE3, 0xFE, 0x95, 0xD4, 0xFE, 0xE7}, // '6'
	{0xE3, 0xFE, 0x95, 0xD4, 0xFE, 0xE7}, // '7'
	{0xE3, 0xFE, 0x95, 0xD4, 0xFE, 0xE7}, // '8'
	{0xE3, 0xFE, 0x95, 0xD4, 0xFE, 0xE7}, // '9'
	*/
};

static const uint8_t fontSmallNumberData[][3] = {
	{0xE3, 0xFE, 0x95}, // '0'
	{0xE3, 0xFE, 0x95}, // '1'
	{0xE3, 0xFE, 0x95}, // '2'
	{0xE3, 0xFE, 0x95}, // '3'
	{0xE3, 0xFE, 0x95}, // '4'
	{0xE3, 0xFE, 0x95}, // '5'
	{0xE3, 0xFE, 0x95}, // '6'
	{0xE3, 0xFE, 0x95}, // '7'
	{0xE3, 0xFE, 0x95}, // '8'
	{0xE3, 0xFE, 0x95}, // '9'
};

static const uint8_t * fontTypeMap[FONT_TYPE_COUNT] = {
	[FONT_TYPE_BIG] = fontBigNumberData[0],
	[FONT_TYPE_SMALL] = fontSmallNumberData[0],
};

/* how many columns occupy one symbol */
const uint8_t fontSymbolSize[FONT_TYPE_COUNT] = {
	[FONT_TYPE_BIG] = sizeof(*fontBigNumberData),
	[FONT_TYPE_SMALL] = sizeof(*fontSmallNumberData),
};

const uint8_t * fontGetSymbolData(uint8_t symbol, FontType font)
{
    return fontTypeMap[font] + (symbol - ASCII_OFFSET) * fontSymbolSize[font]; 

}

uint8_t fontGetSymbolSize(FontType font)
{
	return fontSymbolSize[font];
}

/*
static uint8_t symbol[FONT_SYMBOL_SIZE_IN_BYTES] = {0};

const uint8_t * fontGetNumberArray(uint8_t number)
{
	number &= 0x0F;
	return &fontNumberArray[number][0];
}

const uint8_t * fontGetCharArray(char c)
{
	return &fontCharArray[c - 'A'][0];
}

uint8_t * fontGetNumberArrayShifted(uint8_t number, FontSymbolShift shiftDirection, uint8_t shift)
{
	number &= 0x0F;
	for (uint8_t i = 0; i < FONT_SYMBOL_SIZE_IN_BYTES; i++) {
		switch (shiftDirection) {
		case FONT_SYMBOL_LEFT_SHIFT:
			symbol[i] = fontNumberArray[number][i] << shift;
			break;
		case FONT_SYMBOL_RIGHT_SHIFT:
			symbol[i] = fontNumberArray[number][i] >> shift;
			break;
		default:
			break;
		}
	}
	return symbol;
}

	
uint8_t * fontAddDots(const uint8_t fontSymbol[FONT_SYMBOL_SIZE_IN_BYTES], FontSymbolShift dotsSide)
{
	uint8_t orValue =	switch (dotsSide) {
	case FONT_SYMBOL_LEFT_SHIFT:
		orValue = 0x80;
		break;
	case FONT_SYMBOL_RIGHT_SHIFT:
		orValue = 0x01;
		break;
	default:
		break;
	}
	
	for (uint8_t i = 0; i < FONT_SYMBOL_SIZE_IN_BYTES; i++) {
		if (i == 1 || i == 2 || i == 5 || i == 6)
			symbol[i] = (fontSymbol[i] | orValue);
		else
			symbol[i] = fontSymbol[i];
	}
	return symbol;
}

const uint8_t * fontGetSpaceArray(void)
{
	return fontSpaceArray; 
}
*/

/*
uint8_t * fontGetStringArray(uint8_t columnString[], uint16_t columnStringLen, uint8_t filledRowArray[])
{
	for (uint16_t col = 0; col <= columnStringLen / 8 ; col++) {
		for (uint16_t row = 0; row < 8; row++) {
			for (uint8_t bitNumber = 0; bitNumber < 8; bitNumber++) {
				filledRowArray[row][] |= (1);
			}
		}
		panelIndex++;
	}
}

	for (uint8_t row = 0; row < 8; row++) {
		uint8_t bit = 1 << row;
		for (int8_t i = 7, bit_indx = 0; i >= 0; i--, bit_indx++) {
			uint8_t data =			data = array[i] & bit; // read bit
			data >>= row; // move to 0 position
			data <<= bit_indx; // move to proper position
			if (!data) { // ==			uint8_t data_and_mask = ~(1 << bit_indx); // create mask
				fill_array[row] &= (data | data_and_mask); // write 0 in bit
			} else {
				fill_array[row] |= (data); // write 1 in bit
			}
		}
	}
	
}

*/

/*
function transform array of columns into array of rows, that can be sent to max7219
static void trans_panel(uint8_t *array, uint8_t *fill_array)
{
	for (uint8_t row = 0; row < 8; row++) {
		uint8_t bit = 1 << row;
		for (int8_t i = 7, bit_indx = 0; i >= 0; i--, bit_indx++) {
			uint8_t data =			data = array[i] & bit; // read bit
			data >>= row; // move to 0 position
			data <<= bit_indx; // move to proper position
			if (!data) { // ==			uint8_t data_and_mask = ~(1 << bit_indx); // create mask
				fill_array[row] &= (data | data_and_mask); // write 0 in bit
			}
			else
				fill_array[row] |= (data); // write 1 in bit
		}
    }
}
*/

/*
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
