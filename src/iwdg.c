#include "stm8s.h"
#include "iwdg.h"

#define IWDG_KEY_ENABLE 0xCC
#define IWDG_KEY_REFRESH 0xAA
#define IWDG_KEY_ACCESS 0x55

void iwdgInit(void)
{
    IWDG->KR = IWDG_KEY_ENABLE;
    IWDG->KR = IWDG_KEY_ACCESS;
    IWDG->PR = 0x06; // 110: divider /256
    IWDG->RLR = IWDG_RLR_RESET_VALUE;
    IWDG->KR = IWDG_KEY_REFRESH;
}

void iwdgFeed(void)
{
    IWDG->KR = IWDG_KEY_REFRESH;
}
