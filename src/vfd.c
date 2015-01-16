#include <msp430.h>
#include <stdint.h>
#include "vfd.h"
#include "config.h"
/*
 * Previously called font.h
 *
 *  Partly based on LaydaAda's source code - http://www.ladyada.net/make/icetube/design.html
 */

/*length of time to override the display*/
volatile char overrideTime = 8;

volatile char screen[]   = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
volatile char screenOR[] = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};

const unsigned char specialTable[] = {
    0x02, /* dash */
    0xC6  /* degree */
};

const unsigned char alphaTable[] = {
    0xEE, /* a  0 */
    0x3E, /* b  1 */
    0x1A, /* c  2 */
    0x7A, /* d  3 */
    0xDE, /* e  4 */
    0x8E, /* f  5 */
    0xF6, /* g  6 */
    0x2E, /* h  7 */
    0x60, /* i  8 */
    0x78, /* j  9 */
    0xAE, /* k 10 */
    0x1C, /* l 11 */ 
    0xAA, /* m 12 */
    0x2A, /* n 13 */
    0x3A, /* o 14 */
    0xCE, /* p 15 */
    0xF3, /* q 16 */
    0x0A, /* r 17 */
    0xB6, /* s 18 */
    0x1E, /* t 19 */
    0x38, /* u 20 */
    0x38, /* v 21 */
    0xB8, /* w 22 */
    0x6E, /* x 23 */
    0x76, /* y 24 */
    0xDA, /* z 25 */
    /* more */
};

const unsigned char numberTable[] =
{
    0xFC, /* 0 */
    0x60, /* 1 */
    0xDA, /* 2 */
    0xF2, /* 3 */
    0x66, /* 4 */
    0xB6, /* 5 */
    0xBE, /* 6 */
    0xE0, /* 7 */
    0xFE, /* 8 */
    0xE6, /* 9 */
};

/*
 * Initializes SPI
 * This is used to communicate with the MAX6921 chip
 */
void vfdSpiInit(void)
{
    P2DIR |= VFD_BLANK_PIN | VFD_CS_PIN;
    VFD_BLANK_OFF();
    VFD_DESELECT();

    P1SEL |= SCL_PIN | SDA_PIN;
    P1SEL2 |= SCL_PIN | SDA_PIN;

    UCB0CTL0 |= UCCKPH | UCMSB | UCMST | UCSYNC; // 3-pin, 8-bit SPI master
    UCB0CTL1 |= UCSSEL_2; // SMCLK
    UCB0BR0 |= 0x01; // div/1
    UCB0BR1 = 0;
    UCB0CTL1 &= ~UCSWRST; // Initialize

    //IE2 |= UCB0RXIE;                          // Enable RX interrupt
    DELAY_CYCLES(5000);
}

/*
 * Blanks the display array
 * Clears out display specified by parameter
 */
void vfdDisplayClear(char buffer)
{
    if(buffer==0)
    {
        screen[0] = 0;
        screen[1] = 0;
        screen[2] = 0;
        screen[3] = 0;
        screen[4] = 0;
        screen[5] = 0;
        screen[6] = 0;
        screen[7] = 0;
        screen[8] = 0;
    }
    else if (buffer ==1)
    {
        screenOR[0] = 0;
        screenOR[1] = 0;
        screenOR[2] = 0;
        screenOR[3] = 0;
        screenOR[4] = 0;
        screenOR[5] = 0;
        screenOR[6] = 0;
        screenOR[7] = 0;
        screenOR[8] = 0;
    }
}

char vfdTranslateChar(char c)
{
    if (c >= 97 && c <= 122) //lowercase letters
        return alphaTable[c - 97];
    else if (c >= 65 && c <= 90) //uppercase letters - use same table
        return alphaTable[c - 65];
    else if (c >= 48 && c <= 57) //numbers
        return numberTable[c - 48];
    else if (c == 32) //space
        return 0x00;
    else if (c == 46) //period
        return 1;
    else if (c == '°')
        return specialTable[1];
    else
        return 0x80; //Error character
}

//TODO - having trouble finding length of array - clean up later
void vfdDisplayString(char * c, char len)
{
    uint8_t index = 1;
    while(index - 1 < len) //as long as there are more characters, and we aren't past the edge of our display
    {
        //char toDisp = ;
        screen[index] = vfdTranslateChar(*c++);
        index++;
    }
}
//set an override string and display time
void vfdDisplayORString(char * c, char len, char time)
{
    uint8_t index = 1;
    vfdDisplayClear(1);

    /* As long as there are more characters, and we aren't past the edge of
     * our display*/
    while((index - 1) < len && (index -1 < 9)) 
    {
        char toDisp = *c++;
#if DEBUGTX
        uartTxBuffer[index- 1] = toDisp;
#endif
        screenOR[index] = vfdTranslateChar(toDisp);
        index++;
    }
#if DEBUGTX
    uartTxBufferLen = len;
    uartTx();
#endif

    overrideTime = time;
}

void vfdSetScreen(uint8_t index, char value, char override)
{
    if (override)
        screenOR[index] = value;
    else
        screen[index] = value;
}
/*
 * Outputs a character to the VFD - one digit at a time
 * Used by Interrupt
 * The device accepts 20 bits, send 24 - first 4 bits are discarded
 */
void vfdWrite(char display, char digit)
{
    uint8_t send1;
    uint8_t send2;

    //VFD_BLANK_ON;
    VFD_SELECT();

    if (digit == 8)
    {
        send1 = 0x00;
        send2 = 1<<3;
    }
    else
    {
        send1 = 1<<(7 - digit);
        send2 = 0x00;
    }

    UCB0TXBUF = display>>4;                 //send first 4 bits
    while (!(IFG2 & UCB0TXIFG));            //wait for send to complete
    UCB0TXBUF = (display<<4) | (send1>>4);  //next 8 bits
    while (!(IFG2 & UCB0TXIFG));            //wait for send to complete
    UCB0TXBUF = (send1<<4) | send2;         //last 8 bits
    while (!(IFG2 & UCB0TXIFG));            //wait for send to complete
    VFD_DESELECT();
    //VFD_BLANK_OFF;
}

