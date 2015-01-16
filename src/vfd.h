#ifndef __VFD_H__
#define __VFD_H__
#include "main.h"

void vfdWrite(char address, char data);
void vfdDisplayClear(char buffer);
char vfdTranslateChar(char c);
void vfdDisplayString(char * c, char len);
void vfdDisplayORString(char * c, char len, char time);
void vfdSetScreen(uint8_t index, char value, char override);
void vfdSpiInit(void);

extern volatile char screen[];
extern volatile char screenOR[];
extern volatile char overrideTime;

extern const unsigned char specialTable[];
extern const unsigned char alphaTable[];
extern const unsigned char numberTable[];

#define VFD_BLANK_ON()      P2OUT |= VFD_BLANK_PIN
#define VFD_BLANK_OFF()     P2OUT &= ~VFD_BLANK_PIN
#define VFD_SELECT()        P2OUT &= ~VFD_CS_PIN
#define VFD_DESELECT()      P2OUT |= VFD_CS_PIN


#endif /* HEADER GUARD */
