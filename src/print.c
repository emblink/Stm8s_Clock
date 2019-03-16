#include "stm8s.h"
#include "font.h"

bool print(*uint8_t str, uint16_t len)
{
	if (len > PRINT_MAX_LEN)
		return FALSE;
	
	
}