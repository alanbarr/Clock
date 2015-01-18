#ifndef __BCD_RTC_H__
#define __BCD_RTC_H__

#include <stdint.h>
#include <stdbool.h>

struct btm                                              // BCD time structure
{
    uint8_t sec;
    uint8_t min;
    uint8_t hour;
    uint8_t wday;
    uint8_t mday;
    uint8_t yday;
    uint8_t mon;
    uint16_t year;
};

struct alarm_bcd
{
    uint8_t min;
    uint8_t hour;
    uint8_t daysOfWeek; //allows selection of specific days
};

uint8_t bcdIsLeapYear(unsigned year);
uint8_t alarmCheck(struct btm *t, volatile struct alarm_bcd *a);
void bcdRtcTick(struct btm *t);

#ifdef __GNUC__
inline unsigned short _bcd_add_short(register unsigned short bcd_a,
                                     register unsigned short bcd_b);
#endif


extern const uint8_t bcdDim[18][2];

#endif /*__BCD_RTC_H__*/
