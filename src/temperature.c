/* Previously called one_wire.c */
#include "main.h"
#include "temperature.h"

#if TEMPERATURE_ENABLED
extern volatile unsigned char *owdport, *owiport;
extern unsigned owbit, owtiming;

//temperature defines and variables
volatile char take_temp = 0; //indicates that we should read the temperature
int temp_c, temp_c_1, temp_c_2, temp_f;
/* Always set to a value of TEMP_UNIT_* defines. */
uint8_t tempUnit = TEMPERATURE_UNIT_DEFAULT; 

void one_wire_setup(volatile unsigned char *dir, volatile unsigned char *in,
                    unsigned bitmask, unsigned mhz)
{
    static const unsigned owd[18] = {
    //  us      overhead
        28,     19,     // Zero
        33,     33,
        39,     39,
        7,      14,     // One
        8,      16,
        85,     44,
        500,    23,     // Reset
        50,     25,
        450,    20
    };

    unsigned *p = &owtiming;
    const unsigned *q = owd;
    do {
        *p++ = ((q[0] * mhz) - q[1]) << 1;
        q += 2;
    } while((char *)q < ((char *)owd + sizeof(owd)));
    
    owdport = dir;
    owiport = in;
    owbit = bitmask;
}

//Reads the temperature from the DS18B20
int tempGet(void)
{
    unsigned char b[16];
    int return_val;
    owex(0, 0); owex(0x44CC, 16);                       // Convert
    DELAY_CYCLES(800000);                               // Wait for conversion to complete
    owex(0, 0); owex(0xBECC, 16);                       // Read temperature
    tempReadBlock(b, 9);
    //return_val = b[1]; return_val <<= 8; return_val |= b[0];
    /*
     * b[1] seems to be one of the problem fields - returns 0x01 most of the time,
     * but when it doesn't - it's seriously wrong.  Works for room temp, not sure how it will work for higher or lower temps
     */
    return_val = 0x01; return_val <<= 8; return_val |= b[0];
    //tc = 0x01; tc <<= 8; tc |= b[0];
    return return_val;
}

void tempDisplay(int n, char override, char type)
{
    displayClear(override);
    uint8_t index = 0;
    uint8_t temp = 0;
    char toDisplay_tmp[8];
    for (temp = 0; temp < 8; temp++)
        toDisplay_tmp[temp] = ' ';
    char s[16];
    if(n < 0)
    {
        n = -n;
        toDisplay_tmp[index] = '-';
        index++;
    }
    utoa(n >> 4, s);

    for (temp = 0; temp < 4; temp++)
    {
        if(s[temp])
        {
            toDisplay_tmp[index] = s[temp];
            index++;
        }
        else
            temp = 99; //stop going through string
    }
    //add decimal place
    toDisplay_tmp[index] = '.';
    index++;

    //grab just the value after the decimal place
    n = n & 0x000F;
    if(n)
    {
        utoa(n * 625, s);
        if(n < 2)
        {
            index++;
            toDisplay_tmp[index] = '0';
        }
        for (temp = 0; temp < 2; temp++) //we only have room to print two decimal places
        {
            if(s[temp])
            {

                toDisplay_tmp[index] = s[temp];
                index++;
            }
        }
    }
    else
    {
        toDisplay_tmp[index] = '0';
        toDisplay_tmp[index+1] = '0';
        index += 2;

        //puts("0000");
    }
    toDisplay_tmp[index] = '°';
    toDisplay_tmp[index + 1] = type;//TODO - pass what type of temperature
    if (override)
    {
        displayORString(toDisplay_tmp,8,12);
    }
    else
    {
        displayString(toDisplay_tmp,8);
    }
}

int tempReadBlock(unsigned char *d, unsigned len)
{
    while(len--) {
        *d++ = owex(-1, 8);
    }
    return 0;
}
#endif /* TEMPERATURE_ENABLED */





