#include "stm8s.h"
#include "print.h"
#include "max7219.h"
#include "font.h"

static uint8_t * transpose(const uint8_t data[]);

void print(uint8_t str[], uint16_t len, FontType font)
{
	if (!str)
		return;   

	uint8_t printBuff[PRINT_MAX_LEN] = {0};
    uint16_t buffIdx = 0;
    uint8_t fontSymbolSize = fontGetSymbolSize(font);
    
	for (uint16_t i = 0; i < len; i++) {
        const uint8_t *data = fontGetSymbolData(str[i], font);
        for (uint16_t i = 0; i < fontSymbolSize; i++) {
            printBuff[buffIdx++] = data[i];
        }
	}
    max7219SendDataBuffer(transpose(printBuff), PRINT_MAX_LEN);
}

static uint8_t * transpose(const uint8_t data[])
{
    uint8_t transBuff[PRINT_MAX_LEN] = {0};
    uint16_t i = 0;
    for (uint16_t row = 1; row <= 8; row++) {
        for (uint16_t col = 1; col <= 32; col++) {
            if (col / 8 && col % 8 == 0)
                i++;
            uint8_t dat = data[col - 1];
            static uint8_t mask;
            mask = 0x01 << (8 - row);
            dat &= mask;
            //transBuff[i] |= data[col - 1] & (0x01 << (row - 1));
            transBuff[i] |= dat;
        }
    }
    while (1);
    return transBuff;
}