#ifndef __CONFIG_H__
#define __CONFIG_H__
/********** Configuration parameters **********/

/* Enables (true) / Disables (false) temperature measurement and display. */
#define TEMPERATURE_ENABLED         false /* AB TODO mspgcc it. */

/* Enables serial debugging of printed text if true*/
#define DEBUGTX                     true

/* If set to true the clock uses the optional 32.768 kHz crystal on the launchpad
 * to track the time. If set to false time keeping is done internally, which is
 * less accurate */
#define XTAL                        true

/* Number of alarms */
#define NUM_ALARMS                  2

/* How long to sound an alarm in seconds */
#define ALARM_TIME                  60

/* Default unit of temperature */
#define TEMPERATURE_UNIT_DEFAULT    TEMP_UNIT_CELSIUS

/* Default hour mode */
#define HOUR_MODE_DEFAULT           HOUR_MODE_24

/********** End of Configuration parameters **********/

/******************** Pin Setup ********************/  
#define UART_RX_PIN                 BIT1
#define UART_TX_PIN                 BIT2

#define SCL_PIN                     BIT5 //CLK pin for Max6921
#define SDA_PIN                     BIT7  //DIN pin for MAX6921

#define VFD_CS_PIN                  BIT0 //CS line for the MAX6921
#define VFD_BLANK_PIN               BIT1 //Blank pin for Max6921 - active high

/* Button defines */
#define S1_PIN                      BIT5
#define S2_PIN                      BIT4
#define S3_PIN                      BIT3

/* feature pins - both on Port 1 */
#define BUZ_PIN                     BIT3
#define ONEWIRE_PIN                 BIT4

#define BUZ_OFF()                   P1OUT &= ~BUZ_PIN
#define BUZ_ON()                    P1OUT |= BUZ_PIN
#define BUZ_TOGGLE()                P1OUT |= BUZ_PIN

/* LEDS */
#define LED_RED_PIN                 BIT0
#define LED_RED_ON()                P1OUT |=  LED_RED_PIN
#define LED_RED_OFF()               P1OUT &= ~LED_RED_PIN
#define LED_RED_TOG()               P1OUT ^=  LED_RED_PIN

#define LED_GREEN_PIN               BIT6
#define LED_GREEN_ON()              P1OUT |=  LED_GREEN_PIN
#define LED_GREEN_OFF()             P1OUT &= ~LED_GREEN_PIN
#define LED_GREEN_TOG()             P1OUT ^=  LED_GREEN_PIN

/******************** End of Pin Setup ********************/  

#endif /* HEADER GUARD */

