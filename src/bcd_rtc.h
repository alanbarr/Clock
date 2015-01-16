#ifndef __BCD_RTC_H__
#define __BCD_RTC_H__

#include <stdint.h>
#include <stdbool.h>

struct btm                                              // BCD time structure
{
    /* AB TODO change where possible to uint8_t */
    unsigned int sec;
    unsigned int min;
    unsigned int hour;
    unsigned int wday;
    unsigned int mday;
    unsigned int yday;
    unsigned int mon;
    unsigned int year;
    char time_change; //indicates what values have changed
};

struct alarm_bcd
{
    /* AB TODO change where possible to uint8_t */
    unsigned int min;
    unsigned int hour;
    char daysOfWeek; //allows selection of specific days
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
