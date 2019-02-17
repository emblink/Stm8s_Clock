#ifndef __DS_1307_H
#define __DS_1307_H

#include "stm8s.h"

#define DS1307_ADDRESS (0x68)

bool ds1307_reset(void);
bool ds1307_halt(bool stoped);
bool ds1307_set_seconds(uint8_t sec);
uint8_t ds1307_get_seconds(void);
bool ds1307_set_minutes(uint8_t min);
uint8_t ds1307_get_minutes(void);
bool ds1307_set_hours(uint8_t hour);
uint8_t ds1307_get_hours(void);
bool ds1307_set_day(uint8_t day);
bool ds1307_set_date(uint8_t date);
bool ds1307_set_mounth(uint8_t month);
bool ds1307_set_year(uint8_t year);

#endif // __DS_1307_H