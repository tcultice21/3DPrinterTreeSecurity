#ifndef __INITS_H__
#define __INITS_H__

#include <stdint.h>

#define SYSDIV2 4

void InitPLL(void);
void InitConsole(void);
void InitSystick(void);
void ADC_InitAll(void);
void ADC0_Init(uint32_t pin);
void CAN0_Init(void (*pfnHandler)(void));

#endif // __INITS_H__
