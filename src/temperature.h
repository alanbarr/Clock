/* Previously called one_wire.h */
#ifndef __TEMPERATURE_H__
#define __TEMPERATURE_H__

void one_wire_setup(volatile unsigned char *dir, 
                    volatile unsigned char *in,
                    unsigned bitmask, unsigned mhz);
int owex(int data, unsigned bits);
int tempGet(void);
void tempDisplay(int n, char override,char type);
int tempReadBlock(unsigned char *d, unsigned len);

extern volatile char take_temp;
extern int temp_c, temp_c_1, temp_c_2, temp_f;
extern uint8_t tempUnit;


#endif /*HEADER GUARD*/

