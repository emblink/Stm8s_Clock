#include "stm8s.h"
#include "i2c.h"
#include "DS1307.h"

enum ds1307_register {
    DS1307_SECONDS_REGISTER = 0x00,
    DS1307_MINUTES_REGISTER,
    DS1307_HOURS_REGISTER,
    DS1307_DAY_REGISTER,
    DS1307_DATE_REGISTER,
    DS1307_MONTH_REGISTER,
    DS1307_YEAR_REGISTER,
    DS1307_CONTROL_REGISTER,
    DS1307_REGISTER_COUNT
};

static uint8_t ds1307ResetData[DS1307_REGISTER_COUNT] = {0};

bool ds1307_reset(void)
{
    return i2c_send(DS1307_ADDRESS, DS1307_SECONDS_REGISTER, ds1307ResetData, sizeof(ds1307ResetData));
}

bool ds1307_halt(bool stop)
{
    if (stop) {
        uint8_t ch_bit_mask = 1U << 7;
        return i2c_send(DS1307_ADDRESS, DS1307_SECONDS_REGISTER, &ch_bit_mask, sizeof(ch_bit_mask));
    }
    else {
        return i2c_send(DS1307_ADDRESS, DS1307_SECONDS_REGISTER, 0x00, 1);
    }
}

bool ds1307_set_seconds(uint8_t seconds)
{
    if (seconds > 58)
        return FALSE;
    uint8_t units = seconds % 10;
    uint8_t tens = (seconds / 10) & 0x07;
    seconds = (tens << 4) | units;
    return i2c_send(DS1307_ADDRESS, DS1307_SECONDS_REGISTER, &seconds, sizeof(seconds));
}

bool ds1307_get_seconds(uint8_t *seconds)
{
    bool status = i2c_read(DS1307_ADDRESS, DS1307_SECONDS_REGISTER, seconds, sizeof(*seconds));
    if (status)
        *seconds = (((*seconds & 0x70) >> 4) * 10 + (*seconds & 0x0F));
    return status;
}

bool ds1307_set_minutes(uint8_t minutes)
{
    if (minutes > 59)
        return FALSE;
    uint8_t units = minutes % 10;
    uint8_t tens = (minutes / 10) & 0x07;
    minutes = (tens << 4) | units;
    return i2c_send(DS1307_ADDRESS, DS1307_MINUTES_REGISTER, &minutes, sizeof(minutes));
}

bool ds1307_get_minutes(uint8_t *minutes)
{
    bool status = i2c_read(DS1307_ADDRESS, DS1307_MINUTES_REGISTER, minutes, sizeof(*minutes));
    if (status)
        *minutes = (((*minutes & 0x70) >> 4) * 10 + (*minutes & 0x0F));
    return status;
}

bool ds1307_set_hours(uint8_t hours)
{
    if (hours > 23)
        return FALSE;
    uint8_t units = hours % 10;
    uint8_t tens = (hours / 10) & 0x03;
    hours = (tens << 4) | units;
    return i2c_send(DS1307_ADDRESS, DS1307_HOURS_REGISTER, &hours, sizeof(hours));
}

bool ds1307_get_hours(uint8_t *hours)
{
    bool status = i2c_read(DS1307_ADDRESS, DS1307_HOURS_REGISTER, hours, sizeof(*hours));
    if (status)
        *hours = (((*hours & 0x30) >> 4) * 10 + (*hours & 0x0F));
    return status;
}

bool ds1307_set_day(uint8_t day)
{
    if (day < 1 || day > 7)
        return FALSE;
    day &= 0x07;
    return i2c_send(DS1307_ADDRESS, DS1307_DAY_REGISTER, &day, sizeof(day));
}

bool ds1307_get_day(uint8_t *day)
{
    bool status = i2c_read(DS1307_ADDRESS, DS1307_DAY_REGISTER, day, sizeof(*day));
    if (status)
        *day = (*day & 0x07);
    return status;
}

bool ds1307_set_date(uint8_t date)
{
    if (date < 1 || date > 31)
        return FALSE;
    uint8_t units = (date % 10) & 0x0F;
    uint8_t tens = (date / 10) & 0x03;
    date = ((tens << 4) | units);
    return i2c_send(DS1307_ADDRESS, DS1307_DATE_REGISTER, &date, sizeof(date));
}

bool ds1307_get_date(uint8_t *date)
{
    bool status = i2c_read(DS1307_ADDRESS, DS1307_DATE_REGISTER, date, sizeof(*date));
    if (status)
        *date = (((*date & 0x30) >> 4) * 10 + (*date & 0x0F));
    return status;
}

bool ds1307_set_month(uint8_t month)
{
    if (month < 1 || month > 12)
        return FALSE;
    uint8_t units = (month % 10) & 0x0F;
    uint8_t tens = (month / 10) & 0x01;
    month = (tens << 4) | units;
    return i2c_send(DS1307_ADDRESS, DS1307_MONTH_REGISTER, &month, sizeof(month));
}

bool ds1307_get_month(uint8_t *month)
{
    bool status = i2c_read(DS1307_ADDRESS, DS1307_MONTH_REGISTER, month, sizeof(*month));
    if (status)
        *month = (((*month & 0x10) >> 4) * 10 + (*month & 0x0F));
    return status;
}

bool ds1307_set_year(uint8_t year)
{
    if (year > 99)
        return FALSE;
    uint8_t units = (year % 10) & 0x0F;
    uint8_t tens = (year / 10) & 0x0F;
    year = ((tens << 4) | units);
    return i2c_send(DS1307_ADDRESS, DS1307_YEAR_REGISTER, &year, sizeof(year));
}

bool ds1307_get_year(uint8_t *year)
{
    bool status = i2c_read(DS1307_ADDRESS, DS1307_YEAR_REGISTER, year, sizeof(*year));
    if (status)
        *year = (((*year & 0xF0) >> 4) * 10 + (*year & 0x0F));
    return status;
}