/*
 * enrollment_node.c
 *
 *  Created on: Oct 27, 2019
 *      Author: Carson
 */

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "driverlib/sysctl.h" // Used by Dealy_ms
#include "driverlib/can.h"
#include "inc/hw_memmap.h"
#include "utils/uartstdio.h"
#include "inc/tm4c123gh6pm.h" // CAN STATUS Codes

#include "inits.h"
#include "photon.h"
#include "challenge.h"
#include "FourQ/FourQ_api.h"

static volatile uint32_t g_rec = 0;
static volatile uint32_t g_sent = 0;
static volatile uint32_t g_rec_public = 0;

//static void InitKeys(bool hardcoded, uint8_t *secret_key, uint8_t *ServerPublicKey);
static void Delay_ms(uint32_t ms);

static void CANIntHandler_Enroll(void){
    uint32_t ui32Status = CANIntStatus(CAN0_BASE, CAN_INT_STS_CAUSE);
    if(ui32Status == CAN_INT_INTID_STATUS){
        ui32Status = CANStatusGet(CAN0_BASE, CAN_STS_CONTROL);
    }
    else if(ui32Status == 1){
        // Sent part of response
        CANIntClear(CAN0_BASE, 1);
        g_sent++;
    }
    else if(ui32Status == 2){
        // Received message about type of communication
        //    or confirmation for end of enrollment
        CANIntClear(CAN0_BASE, 2);
        g_rec=1;
    }
    else if(ui32Status == 3){
        // Received part of public key
        CANIntClear(CAN0_BASE, 3);
        g_rec_public++;
    }
}

bool Enrollment(uint8_t node_id, uint8_t *secret_key,
                uint8_t *ServerPublicKey, uint8_t *ec){
    int i;
    uint8_t message[8];
    tCANMsgObject send_obj, recv_obj;
    bool hardcoded;

    memset(message,0,8);

    // Use special handler for enrollment phase
    CANIntRegister(CAN0_BASE, CANIntHandler_Enroll);

    recv_obj.ui32MsgID = 0x411;
    recv_obj.ui32MsgIDMask = 0x7FF;
    recv_obj.ui32Flags = MSG_OBJ_RX_INT_ENABLE | MSG_OBJ_USE_ID_FILTER;
    recv_obj.ui32MsgLen = 1;
    recv_obj.pui8MsgData = (uint8_t*) &hardcoded;

    CANMessageSet(CAN0_BASE, 2, &recv_obj, MSG_OBJ_TYPE_RX);
    while(g_rec == 0){}
    g_rec=0;
    CANMessageGet(CAN0_BASE, 2, &recv_obj, 0);
    CANMessageClear(CAN0_BASE, 2);

    if(hardcoded){
        UARTprintf("Using hardcoded values\n");
        // Set each byte of node response to NODE_ID
        memset(secret_key+16,0,16);
        memset(secret_key,node_id,16);

    }
    else{
        UARTprintf("Generating a response\n");
        // Generate a response
        memset(&secret_key[0],0,32);
        pufResponseEnroll(10, secret_key, ec);
    }

    // Something about reading the ADC messes up CAN
    // Must reinitialize or else CANSet will hard fault
    CAN0_Init(CANIntHandler_Enroll);

    // Wait for the server to send all 4 parts of its public key
    recv_obj.ui32MsgID = 0x100;
    recv_obj.ui32MsgIDMask = 0x7FF;
    recv_obj.ui32Flags = MSG_OBJ_RX_INT_ENABLE | MSG_OBJ_USE_ID_FILTER;
    recv_obj.ui32MsgLen = 8;

    for(i=0; i<3; i++){
        recv_obj.pui8MsgData = &ServerPublicKey[i*8];
        CANMessageSet(CAN0_BASE, 3, &recv_obj, MSG_OBJ_TYPE_RX);
        while(g_rec_public == i){}
        CANMessageGet(CAN0_BASE, 3, &recv_obj, 0);
    }

    // Last portion will be sent specifically to this node
    recv_obj.ui32MsgID = 0x100 + node_id;
    recv_obj.pui8MsgData = &ServerPublicKey[24];
    CANMessageSet(CAN0_BASE, 3, &recv_obj, MSG_OBJ_TYPE_RX);
    while(g_rec_public == i){}
    CANMessageGet(CAN0_BASE, 3, &recv_obj, 0);
    CANMessageClear(CAN0_BASE, 3);

    UARTprintf("Ready to transmit response\n");

    // Send the response (2 packets)
    send_obj.ui32MsgID = 0x200 + node_id;
    send_obj.ui32Flags = MSG_OBJ_TX_INT_ENABLE;
    send_obj.ui32MsgLen = 8;

    send_obj.pui8MsgData = &secret_key[0];
    CANMessageSet(CAN0_BASE, 1, &send_obj, MSG_OBJ_TYPE_TX);
    while(g_sent==0){}
    g_sent=0;
    Delay_ms(30);

    send_obj.pui8MsgData = &secret_key[8];
    CANMessageSet(CAN0_BASE, 1, &send_obj, MSG_OBJ_TYPE_TX);
    while(g_sent==0){}
    g_sent=0;
    CANMessageClear(CAN0_BASE, 1);

    UARTprintf("Last part\n");
    // Wait for final message from server confirming enrollment has completed
    recv_obj.ui32MsgID = 0x411;
    recv_obj.ui32MsgIDMask = 0x7FF;
    recv_obj.ui32Flags = MSG_OBJ_RX_INT_ENABLE | MSG_OBJ_USE_ID_FILTER;
    recv_obj.ui32MsgLen = 1;
    recv_obj.pui8MsgData = message;
    CANMessageSet(CAN0_BASE, 2, &recv_obj, MSG_OBJ_TYPE_RX);

    while(g_rec == 0){}
    g_rec = 0;
    CANMessageGet(CAN0_BASE, 2, &recv_obj, 0);
    CANMessageClear(CAN0_BASE, 2);

    return hardcoded;
}



static void Delay_ms(uint32_t ms){
    SysCtlDelay((ms*80000)/3); // Uses 3 clock cycles per internal loop. 80MHz clk
}
