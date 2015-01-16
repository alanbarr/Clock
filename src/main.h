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

/* Mode defines */
#define TEMP_UNIT_CELSIUS           0
#define TEMP_UNIT_FAHRENHEIT        0

#define HOUR_MODE_24                0
#define HOUR_MODE_12                1

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
extern char settings_mode;          /* AB TODO only used in main */
/* with multiple settings, keeps track of which we're on */
extern volatile char setting_place;  
extern volatile char allow_repeat; /* AB TODO only used in main */

extern volatile char uartTxBuffer[8];
extern volatile uint8_t uartTxBufferLen;

void uartTx(void);



#endif /* HEADER GUARD */
