#include "stm8s.h"
#include "max7219.h"
#include "font.h"

static uint8_t printBuff[PRINT_MAX_LEN];
static uint16_t buffIdx;

static void transposePrintBuff(void);

void print(uint8_t str[], uint16_t len, FontType font);
{
	if (str == NULL)
		return;
	buffIdx = 0;
	for (uint16_t i = 0; i < strLen; i++) {
        uint8_t *data = fontGetSymbolData(str[i], font);
        for (uint16_t i = 0; i < fontSymbolSize[font]; i++) {
            printBuff[buffIdx++] = data[i];
        }
	}
    for (uint16_t i = buffIdx; i < PRINT_MAX_LEN; i++) {
        printBuff[i] = 0x00; // fill rest of the screen
    }
    transposePrintBuff();
    max7219SendData(printBuff, PRINT_MAX_LEN);
}

static void transposePrintBuff(void)
{

}