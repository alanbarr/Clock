#include <msp430.h>
#include <stdint.h>
#include "config.h"
#include "main.h"
#include "bcd_rtc.h"
#include "alarm.h"
#include "vfd.h"

//alarm defines and variables
volatile struct alarm_bcd alarms[NUM_ALARMS];
volatile char alarmsEnabled = 0;  //use bits to enable different alarms
volatile char alarmDuration = 0; //how much longer to sound the alarm
char alarmSnooze = 0;
/* allows iterating through alarms in settings mode */
volatile uint8_t alarmIndex = 0;
#if 0
static const int alarm_on_time = 50;
static const int alarm_off_time = 100;
#endif

static void alarmDisplay(uint8_t alarm_num, char display_type);

void alarmOff(void)
{
    if (alarmDuration > 0)
        alarmSnooze = 1;
}

//Sets up the alarm settings screen when settings entered
void alarmSettingsInit(void)
{
    vfdDisplayClear(1);
    vfdDisplayORString("alarms",6,4);
    settingPlace = 0;
    alarmDisplayAlarms();
}

//Alarm settings - setting 0 screen
void alarmDisplayAlarms(void)
{
    vfdDisplayClear(0);
    char alarm_idx = 0;
    for (alarm_idx = 0; alarm_idx < NUM_ALARMS; alarm_idx++)
    {
        screen[alarm_idx+1] = numberTable[alarm_idx+1];
        if(alarmsEnabled & 1<<alarm_idx)
            screen[alarm_idx+1] |= 1; //turn on decimal place if enabled
    }
}

static void alarmDisplay(uint8_t alarm_num, char display_type)
{
    vfdDisplayClear(0);
    if (display_type == 0) //time only
    {
        //print hour
        if (hourMode == 0) //24h mode
        {
            screen[1] = numberTable[(alarms[alarm_num].hour & 0xF0)>>4];;
            screen[2] = numberTable[(alarms[alarm_num].hour & 0x0F)];
            screen[3] = 1;
        }
        else //12h mode
        {
            if (alarms[alarm_num].hour == 0x12)
            {
                screen[1] = numberTable[(alarms[alarm_num].hour & 0xF0)>>4];
                screen[2] = numberTable[(alarms[alarm_num].hour & 0x0F)];
                //screen[3] = alphaTable[15]; //PM
                screen[0] |= BIT0;//turn on dot
            }
            else if (alarms[alarm_num].hour == 0)
            {
                screen[1] = numberTable[1];
                screen[2] = numberTable[2];
                //screen[3] = alphaTable[0]; //AM
                screen[0] &= ~BIT0;//turn off dot
            }
            else if (alarms[alarm_num].hour > 0x12)
            {
                unsigned int localHour = 0;
                localHour = _bcd_add_short(alarms[alarm_num].hour, 0x9988);
                screen[1] = numberTable[(localHour & 0xF0)>>4];
                screen[2] = numberTable[(localHour & 0x0F)];
                //screen[3] = alphaTable[15]; //PM
                screen[0] |= BIT0;//turn on dot
            }
            else
            {
                screen[1] = numberTable[(alarms[alarm_num].hour & 0xF0)>>4];
                screen[2] = numberTable[(alarms[alarm_num].hour & 0x0F)];
                //screen[3] = alphaTable[0]; //AM
                screen[0] &= ~BIT0;//turn off dot
            }
        }

        screen[4] = numberTable[(alarms[alarm_num].min & 0xF0)>>4];
        screen[5] =numberTable[(alarms[alarm_num].min & 0x0F)];
    }
    else
    {
        vfdDisplayString("smtwtfs",7);
        if (alarms[alarm_num].daysOfWeek & BIT0)
            screen[1] |= 1;
        if (alarms[alarm_num].daysOfWeek & BIT1)
            screen[2] |= 1;
        if (alarms[alarm_num].daysOfWeek & BIT2)
            screen[3] |= 1;
        if (alarms[alarm_num].daysOfWeek & BIT3)
            screen[4] |= 1;
        if (alarms[alarm_num].daysOfWeek & BIT4)
            screen[5] |= 1;
        if (alarms[alarm_num].daysOfWeek & BIT5)
            screen[6] |= 1;
        if (alarms[alarm_num].daysOfWeek & BIT6)
            screen[7] |= 1;
    }
}

//gets fired once a second - lets us blink items when in settings mode
void alarmSettingTick(void)
{
    switch(settingPlace)
    {
        case 0:
            screen[alarmIndex + 1] ^= numberTable[alarmIndex+1];
            break;
        case 1:
            //alarmDisplay(alarmIndex, 0);
            screen[1] ^= 1;
            screen[2] ^= 1;
            break;
        case 2:
            //alarmDisplay(alarmIndex, 0);
            screen[4] ^= 1;
            screen[5] ^= 1;
            break;
        case 3:
            screen[1] ^= alphaTable[18]; //s
            break;
        case 4:
            screen[2] ^= alphaTable[12]; //m
            break;
        case 5:
            screen[3] ^= alphaTable[19]; //t
            break;
        case 6:
            screen[4] ^= alphaTable[22]; //w
            break;
        case 7:
            screen[5] ^= alphaTable[19]; //t
            break;
        case 8:
            screen[6] ^= alphaTable[5];  //f
            break;
        case 9:
            screen[7] ^= alphaTable[18]; //s
            break;
    }
}

uint8_t alarmCheck(struct btm *t, volatile struct alarm_bcd *a)
{
    //check day of the week first
    if (1<<t->wday & a->daysOfWeek)
    {
        if (t->hour == a->hour)
            if(t->min == a->min)
                return true;
    }
    //if we don't satisfy any of the alarm conditions
    return false;
}

void alarmChangeValue(char add)
{
    switch (settingPlace)
    {
        case 1: //add to hour
            vfdDisplayClear(0);
            if(add)
            {
                alarms[alarmIndex].hour = _bcd_add_short(alarms[alarmIndex].hour, 0x1);
                if(alarms[alarmIndex].hour > 0x23)
                    alarms[alarmIndex].hour = 0;
            }
            else
            {
                alarms[alarmIndex].hour = _bcd_add_short(alarms[alarmIndex].hour, 0x9999);
                if(alarms[alarmIndex].hour > 0x23)
                    alarms[alarmIndex].hour = 0x23;
            }
            alarmDisplay(alarmIndex, 0);
            break;
        case 2: //add to minute
            vfdDisplayClear(0);
            if(add)
            {
                alarms[alarmIndex].min = _bcd_add_short(alarms[alarmIndex].min, 0x1);
                if (alarms[alarmIndex].min > 0x59)
                    alarms[alarmIndex].min = 0;
            }
            else
            {
                alarms[alarmIndex].min = _bcd_add_short(alarms[alarmIndex].min, 0x9999);
                if (alarms[alarmIndex].min > 0x59)
                    alarms[alarmIndex].min = 0x59;
            }
            alarmDisplay(alarmIndex, 0);
            break;
        case 3: //Sunday
            alarms[alarmIndex].daysOfWeek ^= BIT0;
            alarmDisplay(alarmIndex, 1);
            break;
        case 4: //Monday
            alarms[alarmIndex].daysOfWeek ^= BIT1;
            alarmDisplay(alarmIndex, 1);
            break;
        case 5: //tuesday
            alarms[alarmIndex].daysOfWeek ^= BIT2;
            alarmDisplay(alarmIndex, 1);
            break;
        case 6: //wednesday
            alarms[alarmIndex].daysOfWeek ^= BIT3;
            alarmDisplay(alarmIndex, 1);
            break;
        case 7: //thursday
            alarms[alarmIndex].daysOfWeek ^= BIT4;
            alarmDisplay(alarmIndex, 1);
            break;
        case 8: //friday
            alarms[alarmIndex].daysOfWeek ^= BIT5;
            alarmDisplay(alarmIndex, 1);
            break;
        case 9: //saturday
            alarms[alarmIndex].daysOfWeek ^= BIT6;
            alarmDisplay(alarmIndex, 1);
            break;
        default: //alarm enable/disable
            vfdDisplayClear(0);
            //1<<alarmIndex
            alarmsEnabled ^= 1<<alarmIndex; //toggle alarm enable state
            alarmDisplayAlarms();
            break;

    }
}

/*
     * Different setting places by mode
     * Alarm Mode
     *      0 = Alarms enable/disable
     *      1 = Hour
     *      2 = Minute
     *      3 = Sunday
     *      4 = Monday
     *      5 = Tuesday
     *      6 = Wednesday
     *      7 = Thursday
     *      8 = Friday
     *      9 = Saturday
     */
void alarmSettingChange(void)
{
    if(settingPlace == 0)
    {
        if (alarmIndex >= (NUM_ALARMS - 1) && alarmIndex < 99)
        {
            alarmIndex = 0;
            settingPlace++;//move on to next setting now that we've covered the alarm enable
        }
        else if (alarmIndex == 99) //just dropped into settings mode
        {
            settingPlace = 0;//basically doing nothing
            alarmIndex = 0;
        }
        else
            alarmIndex++;

    }
    else if(settingPlace == 9)
    {
        if(alarmIndex == NUM_ALARMS - 1)
        {
            settingPlace = 0;
            alarmIndex = 0;
        }
        else
        {
            settingPlace = 1;
            alarmIndex++;
        }
    }
    else
    {
        settingPlace++;
    }
    if (settingPlace == 1)
    {
        vfdDisplayClear(1);
        vfdDisplayORString("alarm ", 6, 8);
        screenOR[7] = numberTable[alarmIndex + 1];
    }
    allowRepeat = 0;
    switch(settingPlace)
    {
        case 0:
            alarmDisplayAlarms();
            break;
        case 1:
            allowRepeat = 1;
            alarmDisplay(alarmIndex, 0);
            screen[1] |= 1;
            screen[2] |= 1;
            break;
        case 2:
            allowRepeat = 1;
            alarmDisplay(alarmIndex, 0);
            screen[4] |= 1;
            screen[5] |= 1;
            break;
        case 3:
            alarmDisplay(alarmIndex, 1);
            //screen[1] |= 1;
            break;
        case 4:
            alarmDisplay(alarmIndex, 1);
            //screen[2] |= 1;
            break;
        case 5:
            alarmDisplay(alarmIndex, 1);
            //screen[3] |= 1;
            break;
        case 6:
            alarmDisplay(alarmIndex, 1);
            //screen[4] |= 1;
            break;
        case 7:
            alarmDisplay(alarmIndex, 1);
            //screen[5] |= 1;
            break;
        case 8:
            alarmDisplay(alarmIndex, 1);
            //screen[6] |= 1;
            break;
        case 9:
            alarmDisplay(alarmIndex, 1);
            //screen[7] |= 1;
            break;
    }
}

