#ifndef __FONT_H
#define __FONT_H

#include "stm8s.h"

#define FONT_SYMBOL_SIZE_IN_BYTES 8
const uint8_t * fontGetNumberArray(uint8_t number);
const uint8_t * fontGetSpaceArray(void);

#endif // __FONT_H




