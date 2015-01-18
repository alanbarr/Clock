/*
 * main.c
 * V2.1.1
 * Christopher Berg
 * Compatible with all revisions of the IV-18 Booster Pack
 *
 * One-Wire Library for DS18B20 based on oPossum's code - http://www.43oh.com/forum/viewtopic.php?f=10&t=1425
 * RTC Code based on oPossum's code - http://www.43oh.com/forum/viewtopic.php?f=10&t=2477&p=21670#p21670
 *
 * Modified by Alan Barr 2015
 * To build with mspgcc and with the intention of making the code more readable.
 *
 */
#include <stdint.h>
#include <msp430.h>
#include "config.h"
#include "main.h"
#include "bcd_rtc.h"
#include "alarm.h"
#include "vfd.h"

#if TEMPERATURE_ENABLED
#include "temperature.h"
#endif

volatile char uartTxBuffer[8];
volatile uint8_t uartTxBufferLen = 0;
static volatile char uartRxBuffer[20];
static volatile uint8_t uartRxBufferLen = 0;

static struct btm time;

//Global settings
static volatile eDisplayMode displayMode = ModeTime;
static volatile uint8_t bufferPlace = 0;
/* Should always be a value of HOUR_MODE_* define */
uint8_t hourMode = HOUR_MODE_DEFAULT;
static uint8_t settingsMode = 0;
/* with multiple settings, keeps track of which we're on */
volatile char settingPlace = 0;
/* if 1, holding down the button will increment values quickly*/
volatile char allowRepeat = 0;

/* Function Prototypes */
static void displayTimerInit(void);
static void clockTimerInit(void);
static void peripheralsInit(void);
static void switchMode(char newMode);
static void timeChangeSetting(void);
static void timeChangeValue(char add);
static void uartInit(void);
static inline void uartRxIsr(void);
static void displayDate(struct btm *t, uint8_t emphasise, uint8_t override);
static void displayTime(struct btm *t, uint8_t emphasise);
static void displayWeekday(unsigned int wday);

#if 0
static volatile char CustomMsg[20] = {'4','3','o','h','.','c','o','m',' ','r','u','l','e','z',0,0,0,0,0,0};
static volatile char CustomMsgLen = 14;
#endif

int main(void) 
{
    WDTCTL = WDTPW + WDTHOLD;

    //initial values for time - incrementing the year would take a long time :)
    time.year = 0x2015;
    time.mday = 0x01;

#if XTAL_PRESENT
    //setting up clocks - 16mhz, using external crystal
    BCSCTL2 = SELM_0 + DIVM_0 + DIVS_2;
#else
    //Using SMCLK divided by 4 for Timer1_A3 (clock tick) source - less accurate than an external crystal
    BCSCTL2 = SELM_0 + DIVM_0 + DIVS_3;
#endif

    DCOCTL = 0;
    BCSCTL1 = CALBC1_16MHZ; // 16MHz clock
    DCOCTL = CALDCO_16MHZ;
    BCSCTL1 |= XT2OFF + DIVA_3;
    BCSCTL3 = XT2S_0 + LFXT1S_0 + XCAP_3;

/* AB TODO Remove? moving to uartInit */
#if 0
    P1OUT = 0x02;
    P1DIR |= 0x02;
#endif

    VFD_BLANK_ON(); //blank display for startup

    //initialization routines
    vfdSpiInit();
    displayTimerInit();
    clockTimerInit();
    uartInit();
    peripheralsInit();

    //clear display
    vfdWrite(0x00,0x00);
    DELAY_CYCLES(10000);    //wait for boost to bring up voltage
    VFD_BLANK_OFF();        //starting writes, turn display on

#if TEMPERATURE_ENABLED
    one_wire_setup(&P1DIR, &P1IN, ONEWIRE_PIN, 16);
    owex(0, 0);         // Reset
    owex(0x33, 8);      // Read ROM
#endif
    //unsigned char b[16];
    //int tc;

    //Testing alarms - alarm on startup
//  alarmsEnabled = 1;
//  alarms[0].daysOfWeek = 0xFF;
//  alarms[0].hour = 0;
//  alarms[0].min = 1;

    switchMode(0);//set up time display

    vfdDisplayORString("startup",7,8);

    __bis_SR_register(LPM0_bits | GIE);

#if DEBUGTX
    uartTxBuffer[0] = 'D';
    uartTxBuffer[1] = 'E';
    uartTxBuffer[2] = 'B';
    uartTxBuffer[3] = 'U';
    uartTxBuffer[4] = 'G';
    uartTxBufferLen = 5;
    uartTx();
#endif

    while (1)
    {
#if TEMPERATURE_ENABLED
        if (take_temp)
        {
            temp_c   = tempGet();
            temp_c_1 = tempGet();
            temp_c_2 = tempGet();

            //take lowest of three values - seems to return the best temperature
            if (temp_c > temp_c_1)
                temp_c = temp_c_1;
            if (temp_c > temp_c_2)
                temp_c = temp_c_2;

            temp_f = temp_c * 9 / 5 + (512);

            if (settingsMode == 0 && displayMode == ModeTemp)
            {
                if (tempUnit == 0)
                    tempDisplay(temp_c,0,'C');
                else
                    tempDisplay(temp_f,0,'F');
            }
            P1OUT &= ~BIT0;
            take_temp = 0;
        }
#endif
        __bis_SR_register(LPM0_bits | GIE);
        //DELAY_CYCLES(800000);
    }
    return 0;
}

//Sets refresh rate for display
static void displayTimerInit(void)
{
    TA0CCTL0 = CM_0 + CCIS_0 + OUTMOD_0 + CCIE;
    TA0CTL = TASSEL_2 + ID_3 + MC_1;
#if XTAL_PRESENT
    //Refresh rate when we're using the crystal
    TA0CCR0 = 1000; //you can change the refresh rate by changing this value
#else
    //This timer is running off of SMCLK too - had to reduce to fix refresh rate issues
    TA0CCR0 = 500; //you can change the refresh rate by changing this value
#endif
}

/*
 * Initializes the interrupt for the RTC functionality and button debouncing
 */
static void clockTimerInit(void)
{
    //Init timer
    TA1CCTL0 = CM_0 + CCIS_0 + OUTMOD_0 + CCIE;
#if XTAL_PRESENT
    //using ACLK as source, divided by 4
    TA1CCR0 = 255;
    TA1CTL = TASSEL_1 + ID_2 + MC_1;
#else
    //using SMCLK as source, divided by 8
    TA1CCR0 = 62499;
    TA1CTL = TASSEL_2 + ID_3 + MC_1;
#endif
}

//Sets up 9600 baud serial connection
//Can be used for the LP's USB/Serial connection, bluetooth serial or GPS module
static void uartInit(void)
{
    P1OUT  |= UART_TX_PIN;
    P1SEL  |= UART_TX_PIN | UART_RX_PIN;
    P1SEL2 |= UART_TX_PIN | UART_RX_PIN;

    UCA0CTL1 |= UCSWRST;
    UCA0CTL1 = UCSSEL_2 + UCSWRST;

#if XTAL_PRESENT
    //Serial settings when we're using the crystal
    UCA0MCTL = UCBRF_0 + UCBRS_6;
    UCA0BR0 = 160;
    UCA0BR1 = 1;
#else
    //different serial settings since NOXTAL modified SMCLK
    UCA0MCTL = UCBRF_0 + UCBRS_3;
    UCA0BR0 = 208;
#endif

    UCA0CTL1 &= ~UCSWRST;
    IE2 |= UCA0RXIE;    // Enable USCI_A0 RX interrupt
    IE2 |= UCA0TXIE;    // Enable TX interrupt for buffering
}

//Configure interrupts on buttons
static void peripheralsInit(void)
{
    P2DIR &= ~(S1_PIN|S2_PIN|S3_PIN);
    P2REN |=  (S1_PIN|S2_PIN|S3_PIN);

#if XTAL_PRESENT == false
    P2SEL &= ~(BIT6 + BIT7); //turn off for xtal pins
#endif

    P1OUT &= ~BUZ_PIN;
    P1DIR |=  BUZ_PIN;

    P1DIR |= LED_GREEN_PIN | LED_RED_PIN; //enable use of LEDS
}

/*
 * Writes the day of week out to the display
 */
static void displayWeekday(unsigned int wday)
{
    switch(wday)
    {
        case 0:
            vfdDisplayString("Mon",3);
            break;
        case 1:
            vfdDisplayString("Tue",3);
            break;
        case 2:
            vfdDisplayString("Wed",3);
            break;
        case 3:
            vfdDisplayString("Thu",3);
            break;
        case 4:
            vfdDisplayString("Fri",3);
            break;
        case 5:
            vfdDisplayString("Sat",3);
            break;
        case 6:
            vfdDisplayString("Sun",3);
            break;
        default:
            vfdDisplayString("err",3);
            break;
    }
}

/*
 * Displays the current time for display
 */
static void displayTime(struct btm *t, uint8_t emphasise)
{
    if (hourMode == HOUR_MODE_12) //subtract 12 before displaying hour
    {
        if (t->hour == 0x12)
        {
            screen[SCREEN_HR_POS] = numberTable[(t->hour & 0xF0)>>4];
            screen[SCREEN_HR_POS+1] = numberTable[(t->hour & 0x0F)];
            screen[0] |= BIT0;//turn on dot
        }
        else if (t->hour == 0)
        {
            screen[SCREEN_HR_POS] = numberTable[1];
            screen[SCREEN_HR_POS+1] = numberTable[2];
            screen[0] &= ~BIT0;//turn off dot
        }
        else if (t->hour > 0x12)
        {
            unsigned int localHour = 0;
            localHour = _bcd_add_short(t->hour, 0x9988);
            screen[SCREEN_HR_POS] = numberTable[(localHour & 0xF0)>>4];
            screen[SCREEN_HR_POS+1] = numberTable[(localHour & 0x0F)];
            screen[0] |= BIT0;//turn on dot
        }
        else
        {
            screen[SCREEN_HR_POS] = numberTable[(t->hour & 0xF0)>>4];
            screen[SCREEN_HR_POS+1] = numberTable[(t->hour & 0x0F)];
            screen[0] &= ~BIT0;//turn off dot
        }
    }
    else
    {
        screen[SCREEN_HR_POS] = numberTable[(t->hour & 0xF0)>>4];
        screen[SCREEN_HR_POS+1] = numberTable[(t->hour & 0x0F)];
        screen[0] &= ~BIT0;//turn off dot
    }

    screen[SCREEN_MIN_POS] = numberTable[(t->min & 0xF0)>>4];
    screen[SCREEN_MIN_POS+1] = numberTable[(t->min & 0x0F)];
    screen[SCREEN_SEC_POS] = numberTable[(t->sec & 0xF0)>>4];
    screen[SCREEN_SEC_POS+1] = numberTable[(t->sec & 0x0F)];
 
    switch (emphasise)
    {
        case SETTINGS_HOUR:
            screen[SCREEN_HR_POS]   |= 1;
            screen[SCREEN_HR_POS+1] |= 1;
            break;
        case SETTINGS_MIN:
            screen[SCREEN_MIN_POS]   |= 1;
            screen[SCREEN_MIN_POS+1] |= 1;
            break;
        case SETTINGS_SEC:
            screen[SCREEN_SEC_POS]   |= 1;
            screen[SCREEN_SEC_POS+1] |= 1;
            break;
        default:
            break;
    }

}

/*
 * If anything special needs to happen between modes, it should be added here
 */
static void switchMode(char newMode)
{
    bufferPlace = 0;
    displayMode = newMode;

    if (displayMode == ModeTime)
    {
        vfdDisplayClear(1);
        vfdDisplayORString("Clock", 5, 8);
    }
    else if (displayMode == ModeAlarm)
    {
        vfdDisplayClear(1);
        vfdDisplayORString("Alarms", 6, 8);
        alarmDisplayAlarms();
    }
#if TEMPERATURE_ENABLED
    else if (displayMode == ModeTemp)
    {
        vfdDisplayClear(1);
        vfdDisplayORString("Temp", 4,8);
        if (tempUnit == TEMP_UNIT_CELSIUS)
            tempDisplay(temp_c,0,'C');
        else
            tempDisplay(temp_f,0,'F');
    }
#endif
    else if (displayMode == ModeText)
    {
        vfdDisplayClear(1);
        vfdDisplayORString("Text", 4,8);
    }
}

/*
 *  Screen refresh timer interrupt
 */
#ifndef __GNUC__
#pragma vector=TIMER0_A0_VECTOR
__interrupt void TIMER0_A0_ISR(void)
#else
interrupt(TIMER0_A0_VECTOR) TIMER0_A0_ISR(void)
#endif
{
    static uint8_t digit = 0;
    char toDisplay;
    
    if (overrideTime > 0)
        toDisplay = screenOR[digit];
    else
        toDisplay = screen[digit];

    vfdWrite(toDisplay, digit);

    digit++;
    if (digit > 8)
    {
        digit = 0;
    }
}

/*
 * Serial TX interrupt
 */
#ifndef __GNUC__
#pragma vector=USCIAB0TX_VECTOR
__interrupt void USCI0TX_ISR(void)
#else
interrupt(USCIAB0TX_VECTOR) USCI0TX_ISR(void)
#endif
{
#if DEBUGTX
    uartTx();//trigger transmission of next character, if any
#endif
}

#if DEBUGTX
void uartTx(void)
{
    static uint8_t TXIndex = 0;

    if (uartTxBufferLen > 0 && uartTxBufferLen < 90) //values above 90 used for other purposes
    {
        IE2 |= UCA0TXIE;                        // Enable USCI_A0 TX interrupt
        UCA0TXBUF = uartTxBuffer[TXIndex];
        TXIndex++;
        if (TXIndex >= uartTxBufferLen)
        {
            TXIndex = 0;
            uartTxBufferLen = 98;
        }
    }
    else if (uartTxBufferLen == 98)
    {
        UCA0TXBUF = '\r';
        uartTxBufferLen = 99;
    }
    else if (uartTxBufferLen == 99)
    {
        UCA0TXBUF = '\n';
        uartTxBufferLen = 0;
        IE2 &= ~UCA0TXIE;                       // Disable USCI_A0 TX interrupt
    }
    else
        TXIndex = 0;
}
#endif

/*
 * Serial RX interrupt
 * Planned use - RX from GPS,
 * PC or connected device to set time, change settings, etc.
 */
#ifndef __GNUC__
#pragma vector=USCIAB0RX_VECTOR
__interrupt void USCI0RX_ISR(void)
#else 
interrupt(USCIAB0RX_VECTOR) USCI0RX_ISR(void)
#endif
{
    //TODO - add some sort of timeout for serial commands
    static char inputState = 0;
    char rx = UCA0RXBUF;

    if (inputState == 0) //ready to rcv a command
    {
        if (rx == '$') //beginning of a transmission
        {
            inputState = 1; //incoming serial transmission
            uartRxBufferLen = 0;
        }
    }
    if (inputState == 1)
    {
        uartRxBuffer[uartRxBufferLen] = rx;
        uartRxBufferLen++;
        if (rx == ';') //end of transmission
        {
            inputState = 2;
        }
    }
    if (inputState == 2)
    {
        uartRxIsr();
        uartRxBufferLen = 0;
        inputState = 0;
    }
//  static char place = 1;
//  static char serialBuffer[] ={0,0,0,0,0,0,0};
//  char waitMode = 1; //waiting for mode input
//
//  char rx = UCA0RXBUF;
//  if (waitMode == 1)
//  {
//      if (rx == 'T')
//          switchMode(0);
//      if (rx == 'M')
//          switchMode(1);
//      waitMode = 0;
//  }
//  else if (displayMode == 0)
//  {
//      serialBuffer[bufferPlace] = rx;
//      if (bufferPlace == 5 || rx == 13)
//      {
//          //buffer filled up - process command
//          unsigned int hour, min;
//          hour = (serialBuffer[0]-48) << 4;
//          hour |= (serialBuffer[1] - 48);
//          min = (serialBuffer[2]-48) << 4;
//          min |= (serialBuffer[3] - 48);
//          time.hour = hour;
//          time.min = min;
//
//            while (!(IFG2&UCA0TXIFG));
//                UCA0TXBUF = 'S';                    // respond as saved
//
//
//          serialBuffer[0] = 0;
//          serialBuffer[1] = 0;
//          serialBuffer[2] = 0;
//          serialBuffer[3] = 0;
//          serialBuffer[4] = 0;
//          bufferPlace = 0;
//          waitMode = 1;
//      }
//      else
//          bufferPlace++;
//  }
//  else if (displayMode == 1)
//  {
//      if (rx >= 97 & rx <= 122) //lowercase letters
//          screen[place] = alphaTable[rx - 97];
//      else if (rx >= 65 & rx <= 90) //uppercase letters - use same table
//          screen[place] = alphaTable[rx - 65];
//      else if (rx >= 48 & rx <= 57) //numbers
//          screen[place] = numberTable[rx - 48];
//      else if (rx == 32) //space
//          screen[place] = 0x00;
//      else if (rx == 46) //period
//      {
//          if (place > 1) //back up if we haven't wrapped to add a period to the previous character
//              place--;
//          screen[place] |= 1;
//      }
//
//      place++;
//      if (place > 8)
//          place = 1;
//  }
}

/*
 * one quarter second timer interrupt (250ms)
 * used for RTC functions, button reading and debouncing and alarm checks
 */
#ifndef __GNUC__
#pragma vector=TIMER1_A0_VECTOR
__interrupt void TIMER1_A0_ISR(void)
#else 
interrupt(TIMER1_A0_VECTOR) TIMER1_A0_ISR(void)
#endif
{
    static unsigned int last_sec = 99;
    static uint8_t count = 0; //how many intervals - 4 = 1s
    static uint8_t S1_Time, S2_Time, S3_Time = 0;

    if (overrideTime > 0)
        overrideTime--;

    count++;

    if (count == 4)
    {
        bcdRtcTick(&time);
        if (displayMode == ModeTime &&
            (settingsMode == 0 ||
             (settingsMode == 1 && (settingPlace >= 1 && settingPlace <= 3))))
        {
            displayTime(&time, 0);
            
            if (settingsMode == 1)
            {
                //set dots based on current setting location
                switch (settingPlace)
                {
                    case 1:
                        screen[1] |= 1;
                        screen[2] |= 1;
                        break;
                    case 2:
                        screen[4] |= 1;
                        screen[5] |= 1;
                        break;
                    case 3:
                        screen[7] |= 1;
                        screen[8] |= 1;
                        break;
                }
            }
            else //toggle dots only when not in settings mode
            {
                screen[3] ^= 1;//toggle dots off and on every second
                screen[6] ^= 1;//toggle dots off and on every second
            }
        }
        else if (displayMode == ModeAlarm && settingsMode == 1)
        {
            alarmSettingTick();
        }

        if (alarmDuration == 0) //don't need to check if alarm is already on
        {
            BUZ_OFF();
            alarmSnooze = 0;
            int alm_num;
            for (alm_num=0;alm_num < NUM_ALARMS; alm_num++)
            {
                if (1<<alm_num & alarmsEnabled) //check to see if alarm is on
                    if(alarmCheck(&time,&alarms[alm_num]))
                {
                    alarmDuration = ALARM_TIME; //turn alarm on
                    vfdDisplayClear(1);
                    vfdDisplayORString("alarm",5, 8);
                    //LPM0_EXIT;
                }
            }
        }
        else
        {
            BUZ_TOGGLE();
            alarmDuration--;

            //LPM0_EXIT;
        }
        count = 0;
    }

    if ((P2IN & S1_PIN) == 0) //Mode
    {
        S1_Time++;
        alarmOff();
    }
    else
    {
        if (S1_Time > 1)
        {
            if (settingsMode == 0) //change mode
            {
                uint8_t NextMode = displayMode + 1;
                if(NextMode >= ModeMax)
                    NextMode = 0;
                vfdDisplayClear(0);
                switchMode(NextMode);
                settingsMode = 0;
            }
            else //in settings mode, this becomes decrement
            {
                if (displayMode == ModeTime)
                    timeChangeValue(0);
                else if (displayMode == ModeAlarm)
                    alarmChangeValue(0);
            }
        }
        S1_Time = 0;
    }
    if ((P2IN & S2_PIN) == 0) //Set
    {
        S2_Time++;
        alarmOff();
    }
    else
    {
        if (S2_Time > 6) //a nice long press will drop us in or out of settings mode
        {
            if (settingsMode == 0)
            {
                settingsMode = 1;
                settingPlace = 0;//move to the first setting
                alarmIndex = 99;
                if (displayMode == ModeAlarm)
                    alarmSettingsInit();
            }
            else if (settingsMode == 1) //exit settings mode
            {
                vfdDisplayClear(0);
                settingsMode = 0;
                if(displayMode == ModeAlarm)
                    alarmDisplayAlarms();
            }
        }
        else if (settingsMode == 1 && S2_Time > 1) //short press toggles through settings
        {

        }
        else if (settingsMode == 0 && S2_Time > 1) //short press, normal mode
        {
            if (displayMode == ModeTime)
            {
                //2sec - display date
                displayDate(&time,0,1);
                overrideTime = 8;
            }
        }
        if (settingsMode == 1 && S2_Time > 1) //in settings mode and a button was pressed
        {
            //go through settings place by mode
            if (displayMode == ModeTime)
            {
                settingPlace++;
                timeChangeSetting();
            }
            else if (displayMode == ModeAlarm)
            {
                alarmSettingChange();
            }
        }
        S2_Time = 0;
    }

    if ((P2IN & S3_PIN) == 0) //Value
    {
        S3_Time++;
        alarmOff();
    }

    if ((P2IN & S3_PIN) != 0 || (S3_Time > 0 && allowRepeat == 1)) //using an if here to allow holding down the button
    {
        if (settingsMode == 1 && S3_Time > 0)
        {
            if (displayMode == ModeTime)
            {
                timeChangeValue(1);
            }
            else if (displayMode == ModeAlarm)
            {
                alarmChangeValue(1);
            }
        }
        else if (settingsMode == 0 && S3_Time > 0)
        {
            if (displayMode == ModeTime)
            {
#if TEMPERATURE_ENABLED
                if (tempUnit == TEMP_UNIT_CELSIUS)
                    tempDisplay(temp_c,1,'C');
                else
                    tempDisplay(temp_f,1,'F');
#endif
            }
        }
        S3_Time = 0;
    }

    //take temp every 30 seconds
    if (time.sec != last_sec)
    {
        last_sec = time.sec;
        //take temperature every 30 seconds
        if (time.sec == 0x30 || time.sec == 0x00)
        {
#if TEMPERATURE_ENABLED 
            take_temp = 1;
            //turn on LED - indicate we're taking the temperature
            LED_RED_ON();
#endif
            __bic_SR_register_on_exit(LPM0_bits); /* Wake up from LPM*/
        }
    }
}

/*
     * Different setting places by mode
     * Time Mode
     *      0 = AM/PM
     *      1 = Hour
     *      2 = Minute
     *      3 = Second
     *      4 = Month
     *      5 = Day
     *      6 = Year
     *      7 = Weekday
     */
static void timeChangeSetting(void)
{
    allowRepeat = 0;
    switch (settingPlace)
    {
        case SETTINGS_HOUR:
            vfdDisplayClear(0);
            vfdDisplayORString("Set Time",8,8);
            allowRepeat = 0x1;
            displayTime(&time, SETTINGS_HOUR);
            break;
        case SETTINGS_MIN:
            vfdDisplayClear(0);
            allowRepeat = 1;
            displayTime(&time, SETTINGS_MIN);
            break;
        case SETTINGS_SEC:
            vfdDisplayClear(0);
            allowRepeat = 1;
            displayTime(&time, SETTINGS_SEC);
            break;
        case SETTINGS_MONTH:
            vfdDisplayClear(0);
            displayDate(&time,SETTINGS_MONTH,0);
            allowRepeat = 1;
            break;
        case SETTINGS_DAY:
            vfdDisplayClear(0);
            displayDate(&time,SETTINGS_DAY,0);
            allowRepeat = 1;
            break;
        case SETTINGS_YEAR:
            vfdDisplayClear(0);
            displayDate(&time,SETTINGS_YEAR,0);
            //allowRepeat = 1;
            break;
        case SETTINGS_WK_DAY:
            vfdDisplayClear(0);
            vfdDisplayORString("Set DOW",7,8);
            displayWeekday(time.wday);
            //allowRepeat = 1;
            break;
        default: //24/12h - 0 or over last setting
            settingPlace = 0; //reset in case setting # went past highest
            vfdDisplayClear(0);
            vfdDisplayORString("Hr Mode",7,8);
            if (hourMode == 0) //24h
            {
                vfdDisplayString("   24h", 6);
            }
            else
            {
                vfdDisplayString("   12h", 6);
            }
            break;
    }
}

static void timeChangeValue(char add)
{
    short daysInMonth = bcdDim[time.mon][bcdIsLeapYear(time.year)];

    switch(settingPlace)
    {
        case SETTINGS_HOUR:
            vfdDisplayClear(0);
            if (add)
            {
                time.hour = _bcd_add_short(time.hour, 0x1);
                if (time.hour > 0x23)
                    time.hour = 0;
            }
            else
            {
                time.hour = _bcd_add_short(time.hour, 0x9999);
                if (time.hour > 0x23)
                    time.hour = 0x23;
            }
            displayTime(&time, SETTINGS_HOUR);
            break;
        case SETTINGS_MIN:
            vfdDisplayClear(0);
            if(add)
            {
                time.min = _bcd_add_short(time.min, 0x1);
                if (time.min > 0x59)
                    time.min = 0;
                time.sec = 0x00;
            }
            else
            {
                time.min = _bcd_add_short(time.min, 0x9999);
                if (time.min > 0x59)
                    time.min = 0x59;
                time.sec = 0x00;
            }
            displayTime(&time, SETTINGS_MIN);
            break;
        case SETTINGS_SEC:
            vfdDisplayClear(0);
            if (add)
            {
                time.sec = _bcd_add_short(time.sec, 1);
                if (time.sec > 0x59)
                    time.sec = 0;
            }
            else
            {
                time.sec = 0;
            }
            displayTime(&time, SETTINGS_SEC);
            break;
        case SETTINGS_MONTH:
            vfdDisplayClear(0);
            if(add)
            {
                time.mon = _bcd_add_short(time.mon, 0x01);
                if (time.mon > 0x11)
                    time.mon = 0x0;
            }
            else
            {
                time.mon = _bcd_add_short(time.mon, 0x9999);
                if (time.mon > 0x11)
                    time.mon = 0x11;
            }
            displayDate(&time,SETTINGS_MONTH,0);
            break;
        case SETTINGS_DAY:
            vfdDisplayClear(0);
            if (add)
            {
                time.mday = _bcd_add_short(time.mday, 1);   // Increment day of month, check for overflow
                if (time.mday > daysInMonth) //if over last day
                    time.mday = 0x01;
            }
            else
            {
                time.mday = _bcd_add_short(time.mday, 0x9999);   // Increment day of month, check for overflow
                if (time.mday == 0) //detect wrap
                    time.mday = daysInMonth;
            }
            displayDate(&time,SETTINGS_DAY,0);
            break;
        case SETTINGS_YEAR:
            vfdDisplayClear(0);
            if (add)
            {
                time.year = _bcd_add_short(time.year,1);
                if (time.year > 0x2099)
                    time.year = 0x2000;
            }
            else
            {
                time.year = _bcd_add_short(time.year,0x9999);
                if (time.year < 0x2000)
                    time.year = 0x2099;
            }
            displayDate(&time,SETTINGS_YEAR,0);
            break;
        case SETTINGS_WK_DAY:
            vfdDisplayClear(0);
            if (add)
            {
                time.wday = _bcd_add_short(time.wday,1);
                if (time.wday > 0x06)
                    time.wday = 0x00;
            }
            else
            {
                time.wday = _bcd_add_short(time.wday,0x9999);
                if (time.wday > 0x06)
                    time.wday = 0x06;
            }
            displayWeekday(time.wday);
            break;
        default://12 or 24h
            vfdDisplayClear(0);
            if (hourMode == 1) //switch to 24h
            {
                hourMode = 0;
                vfdDisplayString("   24h", 6);
            }
            else //switch to 12h
            {
                hourMode = 1;
                vfdDisplayString("   12h", 6);
            }
            break;
    }
}

//Handles display formatting for a date
static void displayDate(struct btm *t, uint8_t emphasise, uint8_t override)
{
    uint8_t month = _bcd_add_short(t->mon,1);//month is 0-11

    vfdSetScreen(SCREEN_DAY_POS, numberTable[(t->mday & 0x00F0)>>4],override);
    vfdSetScreen(SCREEN_DAY_POS+1, numberTable[(t->mday & 0x000F)],override);
    vfdSetScreen(3, specialTable[0],override);
    vfdSetScreen(SCREEN_MON_POS, numberTable[(month & 0x00F0)>>4],override);
    vfdSetScreen(SCREEN_MON_POS+1, numberTable[(month & 0x000F)],override);
    vfdSetScreen(6, specialTable[0],override);
    vfdSetScreen(SCREEN_YEAR_POS, numberTable[(t->year & 0x00F0)>>4],override);
    vfdSetScreen(SCREEN_YEAR_POS+1, numberTable[(t->year & 0x000F)],override);

    switch (emphasise)
    {
        case SETTINGS_DAY:
            screen[SCREEN_DAY_POS]   |= 1;
            screen[SCREEN_DAY_POS+1] |= 1;
            break;
        case SETTINGS_MONTH:
            screen[SCREEN_MON_POS]   |= 1;
            screen[SCREEN_MON_POS+1] |= 1;
            break;
        case SETTINGS_YEAR:
            screen[SCREEN_YEAR_POS]   |= 1;
            screen[SCREEN_YEAR_POS+1] |= 1;
            break;
        default:
            break;
    }
}

/* UART Rx handling. Called from ISR */
static inline void uartRxIsr(void)
{
    char inputType = 0;
    char commandLength = uartRxBufferLen - 1;

    if (uartRxBufferLen > 2)
        inputType = uartRxBuffer[1];//first character after $ is transmission type

    switch (inputType)
    {
        case 'T':
            if (commandLength >= 4) //hours provided
            {
                char h1=0;
                char h2=0;
                if (uartRxBuffer[2] >= 48 && uartRxBuffer[2] <= 57) //ascii 0-9
                    h1 = uartRxBuffer[2] - 48;
                if (uartRxBuffer[3] >= 48 && uartRxBuffer[3] <= 57) //ascii 0-9
                    h2 = uartRxBuffer[3] - 48;
                time.hour = h1<<4 | h2;
            }
            if (commandLength >= 6) //minutes
            {
                char m1=0;
                char m2=0;
                if (uartRxBuffer[4] >= 48 && uartRxBuffer[4] <= 57) //ascii 0-9
                    m1 = uartRxBuffer[4] - 48;
                if (uartRxBuffer[5] >= 48 && uartRxBuffer[5] <= 57) //ascii 0-9
                    m2 = uartRxBuffer[5] - 48;
                time.min = m1<<4 | m2;
            }
            if (commandLength >= 8) //seconds
            {
                char s1=0;
                char s2=0;
                if (uartRxBuffer[6] >= 48 && uartRxBuffer[6] <= 57) //ascii 0-9
                    s1 = uartRxBuffer[6] - 48;
                if (uartRxBuffer[7] >= 48 && uartRxBuffer[7] <= 57) //ascii 0-9
                    s2 = uartRxBuffer[7] - 48;
                time.sec = s1<<4 | s2;
            }
            else
                time.sec = 0; //reset seconds if not provided
            break;
        default:
            //todo - send back error message
            break;
    }
}

