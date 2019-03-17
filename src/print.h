#ifndef __PRINT_H
#define __PRINT_H

#include "stm8s.h"
#include "font.h"

/* column quantity */
#define PRINT_MAX_LEN (4 * 8)// MAX7219_NUMBER_COUNT * FONT_SYMBOL_SIZE_IN_BYTES)

void print(uint8_t str[], uint16_t len, FontType font);

#endif // __PRINT_H