/*
 * main.h
 *
 *  Created on: Sep 21, 2012
 *      Author: cberg
 *
 *  Modified on: 02/01/2015
 *      Author: Alan Barr
 */

#ifndef __MAIN_H__
#define __MAIN_H__

#include <stdint.h>
#include <stdbool.h>
#include "bcd_rtc.h"

#ifdef __GNUC__
    #include "legacymsp430.h"
    #define DELAY_CYCLES(x) __delay_cycles(x)
#else
    #define DELAY_CYCLES(x) _delay_cycles(x)
#endif

#if 0
//assembly functions
void utoa(unsigned, char *);
#endif


/*
 * Mode Values
 * New mode values should be defined here - add on anything you want!
 * Change the ModeMax value to the highest mode to be used
 */
typedef enum {
    ModeTime,
    ModeAlarm,
#if TEMPERATURE_ENABLED
    ModeTemp,
#endif
    ModeText,
    ModeMax,      //highest mode - drop back to 0 after
} eDisplayMode;


extern uint8_t hourMode;
/* with multiple settings, keeps track of which we're on */
extern volatile char settingPlace;  
extern volatile char allowRepeat;

extern volatile char uartTxBuffer[8];
extern volatile uint8_t uartTxBufferLen;

void uartTx(void);


#define SETTINGS_HOUR   1
#define SETTINGS_MIN    2
#define SETTINGS_SEC    3
#define SETTINGS_DAY    4
#define SETTINGS_MONTH  5
#define SETTINGS_YEAR   6
#define SETTINGS_WK_DAY 7

#define SCREEN_1ST_PAIR 1 /* and 2 */
#define SCREEN_2ND_PAIR 4 /* and 5 */
#define SCREEN_3RD_PAIR 7 /* and 7 */

#define SCREEN_HR_POS   SCREEN_1ST_PAIR 
#define SCREEN_MIN_POS  SCREEN_2ND_PAIR
#define SCREEN_SEC_POS  SCREEN_3RD_PAIR

#if DATE_DISPLAY_DEFAULT == DATE_DISPLAY_DEFAULT_DDMMYY
#define SCREEN_DAY_POS  SCREEN_1ST_PAIR 
#define SCREEN_MON_POS  SCREEN_2ND_PAIR
#define SCREEN_YEAR_POS SCREEN_3RD_PAIR

#elif DATE_DISPLAY_DEFAULT == DATE_DISPLAY_DEFAULT_MMDDYY
#define SCREEN_DAY_POS  SCREEN_2ND_PAIR 
#define SCREEN_MON_POS  SCREEN_1ST_PAIR
#define SCREEN_YEAR_POS SCREEN_3RD_PAIR

#else
#define SCREEN_DAY_POS  SCREEN_3RD_PAIR 
#define SCREEN_MON_POS  SCREEN_2ND_PAIR
#define SCREEN_YEAR_POS SCREEN_1ST_PAIR
#endif




#endif /* HEADER GUARD */
