//*****************************************************************************
//
// simple_tx.c - Example demonstrating simple CAN message transmission.
//
// Copyright (c) 2010-2013 Texas Instruments Incorporated.  All rights reserved.
// Software License Agreement
//
//   Redistribution and use in source and binary forms, with or without
//   modification, are permitted provided that the following conditions
//   are met:
//
//   Redistributions of source code must retain the above copyright
//   notice, this list of conditions and the following disclaimer.
//
//   Redistributions in binary form must reproduce the above copyright
//   notice, this list of conditions and the following disclaimer in the
//   documentation and/or other materials provided with the
//   distribution.
//
//   Neither the name of Texas Instruments Incorporated nor the names of
//   its contributors may be used to endorse or promote products derived
//   from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// This is part of revision 1.0 of the Tiva Firmware Development Package.
//
//*****************************************************************************

#include <stdbool.h>
#include <stdint.h>
//#include <inc/hw_can.h>
#include "driverlib/pin_map.h"
#include "inc/tm4c123gh6pm.h"
#include "utils/uartstdio.h"
#include "driverlib/uart.h"
#include "inc/hw_memmap.h"
#include "driverlib/adc.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/can.h"
#include "inits.h"

void InitPLL(void){
	//SysCtlClockSet(SYSCTL_SYSDIV_5 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN | 
	//								SYSCTL_XTAL_16MHZ);

	//Set clock to run at 80 MHz
	SYSCTL_RCC2_R |= 0x80000000;
	SYSCTL_RCC2_R |= 0x00000800;
	SYSCTL_RCC_R = (SYSCTL_RCC_R & ~0x000007C0) + 0x00000540;
	SYSCTL_RCC2_R &= ~0x00000070;
	SYSCTL_RCC2_R &= ~0x00002000;
	SYSCTL_RCC2_R |= 0x40000000;
	SYSCTL_RCC2_R = (SYSCTL_RCC2_R&~0x1FC00000) + (SYSDIV2<<22);
	while((SYSCTL_RIS_R&0x00000040)==0){};
	SYSCTL_RCC2_R &= ~0x00000800;
	
}


void InitConsole(void){
	
	//Enable GPIO port A which is used for UART0 pins
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
	
	//Configure the pins for UART0 functions on port A0 and A1
	GPIOPinConfigure(GPIO_PA0_U0RX);
	GPIOPinConfigure(GPIO_PA1_U0TX);
	
	//Enable UART0 so that we can configure the clock
	SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
	
	//Use the internal 16MHz oscillator as the UART clock source
	UARTClockSourceSet(UART0_BASE, UART_CLOCK_PIOSC);
	
	//Select the alternate (UART) function for these pins
	GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);
	
	//Initialize the UART for console I/O, 9600 BAUD
	UARTStdioConfig(0, 9600, 16000000);
}


void InitSystick(void){
	NVIC_ST_CTRL_R = 0;
	NVIC_ST_RELOAD_R = 0x00FFFFFF;
	NVIC_ST_CTRL_R = 0x5; //Enable with core clock
}

void ADC_InitAll(void){
	volatile int delay;
	SYSCTL_RCGC2_R |= 0x0000001A;
	delay = SYSCTL_RCGC2_R;
	
	/*
	//PB4,5
	GPIO_PORTB_DIR_R &= ~0x30;
	GPIO_PORTB_AFSEL_R |= 0x30;
	GPIO_PORTB_DEN_R &= ~0x30;
	GPIO_PORTB_AMSEL_R |= 0x30;
	*/
	
	//PD0-PD3
	GPIO_PORTD_DIR_R &= ~0x0F;
	GPIO_PORTD_AFSEL_R |= 0x0F;
	GPIO_PORTD_DEN_R &= ~0x0F;
	GPIO_PORTD_AMSEL_R |= 0x0F;
	
	//PE0-PE3
	GPIO_PORTE_DIR_R &= ~0x0F;
	GPIO_PORTE_AFSEL_R |= 0x0F;
	GPIO_PORTE_DEN_R &= ~0x0F;
	GPIO_PORTE_AMSEL_R |= 0x0F;
}

void ADC0_Init(uint32_t pin){
    volatile int delay;
    SYSCTL_RCGC0_R |= 0x00010000;
    delay = SYSCTL_RCGC2_R;
    SYSCTL_RCGC0_R &= ~0x00000300;

    // PC_R is bottom 4 bits
    ADC0_PC_R &= ~0xF;
    ADC0_PC_R |= 0x07; //Sample at 1 MHz

    // 123 makes seq 3 highest priority
    ADC0_SSPRI_R &= ~0x3333;
    ADC0_SSPRI_R |= 0x0123;

    ADC0_ACTSS_R &= ~0x0008;
    ADC0_EMUX_R &= ~0xF000;
    ADC0_SSMUX3_R &= ~0x000F;
    ADC0_SSMUX3_R += pin;

    // SSCTL3 is bottom 4 bits;
    ADC0_SSCTL3_R &= ~0xF;
    ADC0_SSCTL3_R |= 0x0006;

    ADC0_ACTSS_R |= 0x0008;
}


// Initialize CAN0 including interrupts
// *pfnHandler is handler to be called
void CAN0_Init(void (*pfnHandler)(void)){
    // Enable CAN0 on GPIO PORTB pins
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
    GPIOPinConfigure(GPIO_PB4_CAN0RX);
    GPIOPinConfigure(GPIO_PB5_CAN0TX);
    GPIOPinTypeCAN(GPIO_PORTB_BASE, GPIO_PIN_4 | GPIO_PIN_5);

    // Enable CAN peripheral
    SysCtlPeripheralEnable(SYSCTL_PERIPH_CAN0);

    // Initialize the CAN controller
     CANInit(CAN0_BASE);

    // 500 kHz CAN bus rate
    CANBitRateSet(CAN0_BASE, SysCtlClockGet(), 500000);

    // Enable Interrupts
    // Use custom handler if not NULL
    if(pfnHandler){
        CANIntRegister(CAN0_BASE, pfnHandler); // CANIntHandler() used as handler
    }
    CANIntEnable(CAN0_BASE, CAN_INT_MASTER | CAN_INT_ERROR | CAN_INT_STATUS);

    // Enable the CAN for operation.
    CANEnable(CAN0_BASE);
}
