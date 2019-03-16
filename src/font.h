#ifndef __FONT_H
#define __FONT_H

#include "stm8s.h"

typedef enum FontType {
	FONT_TYPE_BIG,
	FONT_TYPE_SMALL,
	FONT_TYPE_COUNT,
} FontType;

/* how many columns occupy one symbol */
const uint8_t fontSymbolSize[FONT_TYPE_COUNT] = {
	[FONT_TYPE_BIG] = 6,
	[FONT_TYPE_SMALL] = 3,
};

typedef enum SymbolShift {
	FONT_SYMBOL_LEFT_SHIFT,
	FONT_SYMBOL_RIGHT_SHIFT,
} FontSymbolShift;

const uint8_t * fontGetNumberArray(uint8_t number);
const uint8_t * fontGetCharArray(char c);
const uint8_t * fontGetSpaceArray(void);
uint8_t * fontGetNumberArrayShifted(uint8_t number, FontSymbolShift shiftDirection, uint8_t shift);
uint8_t * fontAddDots(const uint8_t fontSymbol[FONT_SYMBOL_SIZE_IN_BYTES], FontSymbolShift dotsSide);
uint8_t * fontGetStringArray(uint8_t buffer[], uint16_t bufferLen);
#endif // __FONT_H




