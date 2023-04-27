#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "driverlib/pin_map.h"
#include "inc/tm4c123gh6pm.h"
#include "utils/uartstdio.h"
#include "driverlib/uart.h"
#include "inc/hw_memmap.h"
#include "driverlib/adc.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "challenge.h"
#include "present.h"
#include "inits.h"

#define TEST 0          // Set 0 for reading, 1+ for hardcoded values
#define READS 100000    // Number of reads to average per input

static unsigned long avg_amp(uint32_t pin);
static unsigned long ADC0_In(uint32_t pin);
static void compare(unsigned long *copy, unsigned long *l, unsigned long *r, uint8_t *ret);
static void arr(unsigned long *val,int a, int b, int c);

static int place;

// # pin                    GND
// Pin 0 is PE3     R0
// Pin 1 is PE2     R1
// Pin 2 is PE1     R2
// Pin 3 is PE0     R3
// Pin 4 is PD3     R4
// Pin 5 is PD2     R5
// Pin 6 is PD1     R6
// Pin 7 is PD0     R7 VDD


//res should be a buffer of total size 16 bytes
void pufResponse(uint8_t *res){
    unsigned long raw[8];
    unsigned long copy[8];
    unsigned long left[3];
    unsigned long right[3];
    int i;

    memset(res,0,16);
    place=0;

    if(TEST==0){
        for(i=0;i<8;i++){
            raw[i] = avg_amp(i);
            //copy[i]=raw[i];
        }

        //Extract the actual voltage readings
        //copy[7]=4095-raw[6];
        copy[7]=raw[7]-raw[6];
        copy[6]=raw[6]-raw[5];
        copy[5]=raw[5]-raw[4];
        copy[4]=raw[4]-raw[3];
        copy[3]=raw[3]-raw[2];
        copy[2]=raw[2]-raw[1];
        copy[1]=raw[1]-raw[0];
        copy[0]=raw[0];
    }

    else if(TEST == 1){
        // Hardcoded values for testing
        copy[0]=3839;
        copy[1]=3955;
        copy[2]=3957;
        copy[3]=3957;
        copy[4]=3976;
        copy[5]=3972;
        copy[6]=3969;
        copy[7]=3924;
    }
    else if(TEST == 2){
        // More hardcoded values for testing
        copy[0]=3879;
        copy[1]=3961;
        copy[2]=3959;
        copy[3]=3960;
        copy[4]=3977;
        copy[5]=3981;
        copy[6]=3973;
        copy[7]=3959;
    }

    arr(left,1,2,3);
    arr(right,4,5,6);
    compare(copy,left,right,&res[0]); //1

    arr(left,1,2,4);
    arr(right,3,5,6);
    compare(copy,left,right,&res[1]); //2

    arr(left,1,2,5);
    arr(right,3,4,6);
    compare(copy,left,right,&res[2]); //3

    arr(left,1,2,6);
    arr(right,3,4,5);
    compare(copy,left,right,&res[3]); //4

    arr(left,1,2,7);
    arr(right,3,4,5);
    compare(copy,left,right,&res[4]); //5

    arr(left,1,2,8);
    arr(right,3,4,5);
    compare(copy,left,right,&res[5]); //6

    arr(left,1,3,4);
    arr(right,2,5,6);
    compare(copy,left,right,&res[6]); //7

    arr(left,1,3,5);
    arr(right,2,4,6);
    compare(copy,left,right,&res[7]); //8

    arr(left,1,3,6);
    arr(right,2,4,5);
    compare(copy,left,right,&res[8]); //9

    arr(left,1,3,7);
    arr(right,2,4,5);
    compare(copy,left,right,&res[9]); //10

    arr(left,1,3,8);
    arr(right,2,4,5);
    compare(copy,left,right,&res[10]); //11

    arr(left,1,4,5);
    arr(right,2,3,6);
    compare(copy,left,right,&res[11]); //12

    arr(left,1,4,6);
    arr(right,2,3,7);
    compare(copy,left,right,&res[12]); //13

    arr(left,1,4,7);
    arr(right,2,3,8);
    compare(copy,left,right,&res[13]); //14

    arr(left,1,4,8);
    arr(right,2,3,5);
    compare(copy,left,right,&res[14]); //15

    arr(left,1,5,6);
    arr(right,2,3,4);
    compare(copy,left,right,&res[15]); //16
}

// res and ec should each be 16 byte buffers
// ec will be error correction to OR with future responses
void pufResponseEnroll(uint8_t num_reads, uint8_t *res, uint8_t *ec){
    // Generate num_reads of responses
    // Track in ec the bit locations that change from first response
    // Those bits can be OR'ed with future responses to correct bit flips

    // We want ec to be a bit mask of toggling bits
    // We don't want it to be every high bit from num_reads responses
    // The later is not much better than just storing the original response


    //volatile int i;

    int i;
    pufResponse(res);
    //printResponse(res);
    memset(ec,0,16);

    // ec will be all 0's
    if(num_reads==1){
        return;
    }
    else{
        uint8_t first[16]; // Holds the first response
        memmove(first,res,16);

        for(i=1; i<num_reads; i++){
            int j;
            UARTprintf("%i remain\n",num_reads-i);
            pufResponse(res);
            //printResponse(res);
            for(j=0; j<16; j++){
                ec[j] = ec[j] | (first[j] ^ res[j]);
            }
        }
    }

    // Make sure the response ultimately used by enrollment
    // has been corrected
    for(i=0; i<16; i++){
        res[i] = res[i] | ec[i];
    }

    // Check ECC
    printResponse(ec);
}

// Generates a response (res) and ORs with the correction code (ec)
void pufResponseCorrection(uint8_t *res, uint8_t *ec){
    int i;
    pufResponse(res);
    for(i=0; i<16; i++){
        res[i] |= ec[i];
    }
}

void printResponse(uint8_t *res){
    int i;
    UARTprintf("0x");
    for(i=15; i>=0; i--){
        UARTprintf("%02x",res[i]);
    }
    UARTprintf("\n");
}



static unsigned long avg_amp(uint32_t pin){
    unsigned long total;
    int i;
    total=0;
    for(i=0;i<READS;i++){
        total += ADC0_In(pin);
    }
    total = total/READS;
    return total;
}

static unsigned long ADC0_In(uint32_t pin){
    unsigned long result;
    ADC0_Init(pin);

    ADC0_PSSI_R &= ~0xF;
    ADC0_PSSI_R = 0x0008;

    while((ADC0_RIS_R&0x08)==0){};
    result = ADC0_SSFIFO3_R&0xFFF;

    ADC0_ISC_R |= 0x0008;
    //ADC0_ISC_R = 0x0008;
    return result;
}

static void compare(unsigned long *copy, unsigned long *l, unsigned long *r, uint8_t *ret){
    int i;
    unsigned long left,right;
    *ret=0;
    left=0;
    right=0;
    for(i=0;i<3;i++){
        l[i]--;
        r[i]--;
    }
    for(i=0;i<8;i++){
        left=copy[(i+l[0])%8]+copy[(i+l[1])%8]+copy[(i+l[2])%8];
        right=copy[(i+r[0])%8]+copy[(i+r[1])%8]+copy[(i+r[2])%8];
        if(left>right){*ret |= (1 << i);}
        place++;
    }
}

static void arr(unsigned long *val,int a, int b, int c){
    val[0]=a;
    val[1]=b;
    val[2]=c;
}
