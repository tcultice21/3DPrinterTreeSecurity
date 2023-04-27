#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "driverlib/adc.h"
#include "driverlib/sysctl.h"
#include "driverlib/pin_map.h"
#include "driverlib/gpio.h"
#include "driverlib/uart.h"
#include "driverlib/can.h"
#include "inc/hw_memmap.h"
#include "inc/tm4c123gh6pm.h"
#include "utils/uartstdio.h"
#include "inits.h"

#include "enrollment_node.h"
#include "challenge.h"
#include "present.h"
#include "photon.h"
#include "FourQ/FourQ_api.h"

#define DEBUG       0   // Print debug statements
#define NODE_ID     2   // Node ID to be flashed.  Node 0 is server
#define NODE_TOTAL  2   // # of nodes not counting server
#define TOTAL_SENDS 10  // Messages to send before restarting with Authentication

// # pin			GND
// Pin 0 is PE3		R0
// Pin 1 is PE2		R1
// Pin 2 is PE1		R2
// Pin 3 is PE0		R3
// Pin 4 is PD3		R4
// Pin 5 is PD2		R5
// Pin 6 is PD1		R6
// Pin 7 is PD0		R7 VDD

//void CANIntHandler(void);

//unsigned char ServerPublicKey[32];
//static uint8_t ServerPublicKey[32];

static volatile uint32_t g_bErrFlag = 0;

// Signifies that first half of received message has been copied over
static volatile uint32_t g_recComplete = 0;

// Flag for sending hashed response
static volatile uint32_t g_resSend = 0;

// Flag for receiving shared key
static volatile uint32_t g_sharedReceived = 0;

// Flag set to 1 when authentication has completed
static volatile uint32_t g_normalOp = 0;

// Flag tracking order to send and receive
static volatile uint32_t g_waitFlag = 0;

// Flag tracking when all nodes have been authenticated
static volatile uint32_t g_normalFlag = 0;

// Flag tracking normal message received
static volatile uint32_t g_received = 0;

// Flag tracking normal message sent
static volatile uint32_t g_sent = 0;

static void resetFlags(void);
static void Delay_ms(uint32_t ms);


void CANIntHandler_Auth(void){
    uint32_t ui32Status = CANIntStatus(CAN0_BASE, CAN_INT_STS_CAUSE);
    //UARTprintf("TX: 0x%04x\n",CANStatusGet(CAN0_BASE, CAN_STS_TXREQUEST));
    //UARTprintf("NEWDAT: 0x%04x\n",CANStatusGet(CAN0_BASE, CAN_STS_NEWDAT));
    //UARTprintf("INT_STS: 0x%04x\n",ui32Status);
    // Error occurred?
    // Ugly as sin but keep reading until we don't get RXOK or TXOK
    while(ui32Status == CAN_INT_INTID_STATUS){
        ui32Status = CANStatusGet(CAN0_BASE, CAN_STS_CONTROL);
        if((ui32Status == 0x10) || (ui32Status == 0x08)){
            //UARTprintf("STS_CONTROL: %04x\n",CANStatusGet(CAN0_BASE, CAN_STS_CONTROL));
            ui32Status = CANIntStatus(CAN0_BASE, CAN_INT_STS_CAUSE);
            //UARTprintf("New INT_STS: %04x\n",ui32Status);
        }
        else{
            g_bErrFlag = 1;
            return;
        }
    }

    // Want monitor (object 3) to always be on during authentication so this ignores
    // the monitor receiving messages that are needed by others
    if((ui32Status == 0) && (CANStatusGet(CAN0_BASE, CAN_STS_NEWDAT) & 0x04)){ui32Status=3;}

    // Sending hashed response
    if(ui32Status == 1){
        if(g_resSend==0){
            g_resSend=1;
        }
        else if(g_resSend==1){
            g_resSend++;
        }
        CANIntClear(CAN0_BASE, 1);
    }

    // Receiving shared secret
    else if(ui32Status == 2){
        CANIntClear(CAN0_BASE, 2);
        if(g_sharedReceived==0){
            g_sharedReceived++;
        }
        // Don't process until it says it's ready
        else if((g_sharedReceived == 1) && (g_recComplete==1)){
            g_sharedReceived++;
        }
    }

    else if(ui32Status == 3){
        CANIntClear(CAN0_BASE, 3);
        g_waitFlag++;
    }

    else if(ui32Status == 4){
        CANIntClear(CAN0_BASE, 4);
        g_normalFlag++;
    }

    else if(ui32Status == 6){
            CANIntClear(CAN0_BASE, 6);
            g_normalFlag++;
        }

    Delay_ms(1);

    // Normal Operation Time


    // How did this happen
    //else{UARTprintf("This should not happen!!!\n");}
}

void CANIntHandler_Normal(void){
    uint32_t ui32Status = CANIntStatus(CAN0_BASE, CAN_INT_STS_CAUSE);
    if(ui32Status == CAN_INT_INTID_STATUS){
        ui32Status = CANStatusGet(CAN0_BASE, CAN_STS_CONTROL);
    }
    else if(ui32Status == 1){
        CANIntClear(CAN0_BASE, 1);
        g_received++;
    }
    else if(ui32Status == 2){
        CANIntClear(CAN0_BASE, 2);
        g_sent=0;
    }
    else{
        UARTprintf("Normal shouldn't reach this!\n");
    }
}

int main(void){
	volatile ECCRYPTO_STATUS status;
	int i;
	volatile char button;
	bool hardcoded;

	uint8_t secret_key[32];
	uint8_t public_key[32];
	uint8_t shared_secret[32];
	uint8_t ServerPublicKey[32];
	uint8_t response[16];
	uint8_t ec[16];
	uint8_t session_key[10];

	uint8_t message[16]; // Buffer for CAN messaging
	uint8_t message_in[8];
	uint8_t message_out[8];
	uint8_t response_hash[16];
	uint8_t shared_hash[16]; // Used as key during authentication. Present only uses first 10 bytes
	uint8_t encrypted_response_hash[16]; // Will be sent to server
	uint8_t server_reply[16]; // Reply from server containing shared encryption secret and valid nodes
	ECCRYPTO_STATUS Status;

	tCANMsgObject msg_obj_1, msg_obj_2, msg_obj_wait, msg_obj_mon, msg_obj_send, msg_obj_rec;

	InitPLL();
	InitConsole();
	ADC_InitAll();
	CAN0_Init(NULL);

	UARTprintf("Time for enrollment\n");

	// Generate response using server's instructions
	// Send response to server and receive server's public key

	/*
	 *
	 *  TODO:   Add a copy of the stored hash so that each newly generated
	 *          response can be compared against it for error checking.
	 *          You would keep generating responses until one passed
	 */

	hardcoded = Enrollment(NODE_ID, secret_key, ServerPublicKey, ec);

	UARTprintf("Enrollment complete\n");

	// Infinite loop to repeat Authentication and Normal Operation
	while(1){
	    resetFlags();

        // Set CAN to use authentication handler
        CAN0_Init(CANIntHandler_Auth);
        UARTprintf("Initialization Complete...\n");
        UARTprintf("Trying to test this thing...\n");

        // Generate the node's response and keys
        //InitKeys(hardcoded,response,secret_key,public_key);
        memset(secret_key+16,0,16);

        if(hardcoded){
            UARTprintf("Hardcoding Response\n");
            // Set each byte of node response to NODE_ID
            memset(response,NODE_ID,16);
        }
        else{
            UARTprintf("Generating Response\n");
            // Generate response
            pufResponseCorrection(response,ec);
            if(NODE_ID == 1){
                Delay_ms(2000); // Let the server generate its own
            }
        }

        memmove(secret_key,response,16);

        // Something about reading the ADC messes up CAN
        // Must reinitialize or else CANSet will hard fault
        CAN0_Init(CANIntHandler_Auth);

        // Hash the response to send to the server
        // secret_key's lowest 16 bytes contain the PUF's response
        photon128(response,16,response_hash);

        if(DEBUG){
            int j;

            UARTprintf("Hash is 0x");
            for(j=15; j>=0; j--){
                UARTprintf("%02x",response_hash[j]);
            }
            UARTprintf("\n");
        }

        // Generate the node's public key
        Status = ECCRYPTO_SUCCESS;
        Status = CompressedKeyGeneration(secret_key, public_key);
        if (Status != ECCRYPTO_SUCCESS) {
            UARTprintf("Failed Public Key Generation\n");
            return Status;
        }

        // Generate shared secret
        Status = CompressedSecretAgreement(secret_key, ServerPublicKey, shared_secret);
        if (Status != ECCRYPTO_SUCCESS) {
            UARTprintf("Failed Shared Secret Generation\n");
            return Status;
        }

        // Hash 32-byte shared secret to 128 bits then truncate to 10 bytes
        // Only first 10 bytes of shared_hash will be used
        photon128(shared_secret,32,shared_hash);

        // Encrypt the two halves of the response_hash (8 bytes at a time)
        // The first 10 bytes of shared_hash is the key
        memmove(encrypted_response_hash,response_hash,16);
        present80Encrypt(shared_hash,encrypted_response_hash);
        present80Encrypt(shared_hash,&encrypted_response_hash[8]);


        /*
         *
         * TODO:    Move the CAN initialization to the start of authentication
         *          so that a node that needs multiple attempts to generate a
         *          response does not miss a message
         *
         */
        // Enable Monitoring
        msg_obj_mon.ui32MsgID = 0x301;
        msg_obj_mon.ui32MsgIDMask = 0x7FF;
        msg_obj_mon.ui32Flags = MSG_OBJ_RX_INT_ENABLE | MSG_OBJ_USE_ID_FILTER;
        msg_obj_mon.ui32MsgLen = 8;
        msg_obj_mon.pui8MsgData = message;
        CANMessageSet(CAN0_BASE, 6, &msg_obj_mon, MSG_OBJ_TYPE_RX);

        // Wait your turn
        msg_obj_wait.ui32MsgID = 0;
        msg_obj_wait.ui32MsgIDMask = 0;
        msg_obj_wait.ui32Flags = MSG_OBJ_RX_INT_ENABLE | MSG_OBJ_USE_ID_FILTER;
        msg_obj_wait.ui32MsgLen = 8;
        msg_obj_wait.pui8MsgData = message;
        CANMessageSet(CAN0_BASE, 3, &msg_obj_wait, MSG_OBJ_TYPE_RX);

        UARTprintf("Waiting\n");

    // Prevents warning from 1st node comparing unsigned int with 0
#if NODE_ID > 1
        while(g_waitFlag < 2*(NODE_ID-1)){}
#else
        // Node 1 needs to give server enough time to generate
        // all shared secrets
        if(DEBUG){
            // There is a lot for the server to print
            Delay_ms(NODE_TOTAL * 1100);
        }
        else{
            Delay_ms(NODE_TOTAL * 500);
        }
#endif

        // It's your turn so stop listening
        CANMessageClear(CAN0_BASE, 3);
        UARTprintf("My Turn!\n");

        // Send to the server
        // Set to send CAN0 msg 1;
        // msg_obj_one.ui32MsgID = 0x1001;
        Delay_ms(50);
        msg_obj_1.ui32MsgID = 0x100 + NODE_ID;
        //msg_obj_1.ui32MsgIDMask = 0x100 + NODE_ID;
        msg_obj_1.ui32MsgIDMask = 0x7FF;
        msg_obj_1.ui32Flags = MSG_OBJ_TX_INT_ENABLE | MSG_OBJ_USE_ID_FILTER;
        msg_obj_1.ui32MsgLen = 8;
        msg_obj_1.pui8MsgData = encrypted_response_hash;
        CANMessageSet(CAN0_BASE, 1, &msg_obj_1, MSG_OBJ_TYPE_TX);

        while(g_resSend != 1){} // Wait for first half to send
        UARTprintf("First message sent!\n");
        Delay_ms(50);

        // Send the second half
        msg_obj_1.pui8MsgData = &encrypted_response_hash[8];
        CANMessageSet(CAN0_BASE, 1, &msg_obj_1, MSG_OBJ_TYPE_TX);
        while(g_resSend != 2){} // Wait for second half to send
        CANIntClear(CAN0_BASE,1);
        UARTprintf("Second message sent!\n");
        g_waitFlag += 2;

        // Wait for the other nodes to send their hashed responses
        CANMessageSet(CAN0_BASE, 3, &msg_obj_wait, MSG_OBJ_TYPE_RX);
        UARTprintf("Waiting for others\n");
        while(g_waitFlag < 2*(NODE_TOTAL)){}
        CANMessageClear(CAN0_BASE, 3);
        CANIntClear(CAN0_BASE,3);
        UARTprintf("All nodes finished sending hashed responses\n");


        // Set CAN0 message object 2 to receive
        msg_obj_2.ui32MsgID = 0x200 + NODE_ID;
        //msg_obj_2.ui32MsgIDMask = 0x200 + NODE_ID;
        msg_obj_2.ui32MsgIDMask = 0x7FF;
        msg_obj_2.ui32Flags = MSG_OBJ_RX_INT_ENABLE | MSG_OBJ_USE_ID_FILTER;
        msg_obj_2.ui32MsgLen = 8;
        msg_obj_2.pui8MsgData = server_reply;
        CANMessageSet(CAN0_BASE, 2, &msg_obj_2, MSG_OBJ_TYPE_RX);

        // Wait for server response containing shared secret
        while(g_sharedReceived==0){}
        CANMessageGet(CAN0_BASE, 2, &msg_obj_2, 0);
        msg_obj_2.pui8MsgData = &server_reply[8];
        CANMessageSet(CAN0_BASE, 2, &msg_obj_2, MSG_OBJ_TYPE_RX);
        g_recComplete++;
        UARTprintf("Received first message\n");

        // Wait for second half of server response to arrive
        while(g_sharedReceived==1){}
        UARTprintf("Received second message\n");
        CANMessageGet(CAN0_BASE, 2, &msg_obj_2, 0);

        UARTprintf("Msg ID=0x%04x received: ",msg_obj_2.ui32MsgID);
        for(i=0;i<16;i++){UARTprintf("%02x ",server_reply[i]);}
        UARTprintf("\n");

        // Decrypt shared secret
        present80Decrypt(shared_hash,server_reply);
        present80Decrypt(shared_hash,&server_reply[8]);

        memmove(session_key,server_reply,10);

        UARTprintf("Finished Decrypting!\n");
        UARTprintf("Session key is: 0x");
        for(i=9;i>=0;i--){UARTprintf("%02x",session_key[i]);}

        // Set up for normal communication with other ECUs
        while(g_normalFlag==0){}
        CANDisable(CAN0_BASE);
        CANIntClear(CAN0_BASE,1);
        CAN0_Init(CANIntHandler_Normal);
        CANEnable(CAN0_BASE);
        UARTprintf("\nNormal Operation Time\n");

        // Receive on 1
        msg_obj_rec.ui32MsgID = 0x400 + NODE_ID;
        //msg_obj_1.ui32MsgIDMask = 0x100 + NODE_ID;
        msg_obj_rec.ui32MsgIDMask = 0x7F0;
        msg_obj_rec.ui32Flags = MSG_OBJ_RX_INT_ENABLE | MSG_OBJ_USE_ID_FILTER;
        msg_obj_rec.ui32MsgLen = 8;
        msg_obj_rec.pui8MsgData = message_in;
        CANMessageSet(CAN0_BASE, 1, &msg_obj_rec, MSG_OBJ_TYPE_RX);

        // Send on 2
        msg_obj_send.ui32MsgID = 0x400 + NODE_ID;
        msg_obj_send.ui32MsgIDMask = 0x7FF;
        msg_obj_send.ui32Flags = MSG_OBJ_TX_INT_ENABLE;
        msg_obj_send.ui32MsgLen = 8;
        msg_obj_send.pui8MsgData = message_out;

        memset(message_out,0,8);
        Delay_ms(1000);

        if(NODE_ID == 1){
            present80Encrypt(session_key,message_out);
            CANMessageSet(CAN0_BASE, 2, &msg_obj_send, MSG_OBJ_TYPE_TX);
            g_sent=1;
            Delay_ms(1000);
        }

        // Send and received a fix number of messages until restarting
        // with the authentication phase
        for(i=0; i<TOTAL_SENDS; i++){
            while(g_sent != 0){}

            while(g_received == 0){}
            //if(g_received != 0){
                //Delay_ms(250);
                g_received=0;
                CANMessageGet(CAN0_BASE, 1, &msg_obj_rec,0);
                present80Decrypt(session_key,message_in);
                UARTprintf("Received: 0x%08x.%08x\n",*((uint32_t *)&message_in[4]),*((uint32_t *)message_in));
                *((uint32_t *)message_in) += NODE_ID;
                memmove(message_out,message_in,8);

                Delay_ms(500);

                // Node 1 will do its first send before the for loop
                // Therefore it shouldn't send on the last loop iteration
                if((i<TOTAL_SENDS-1) || (NODE_ID != 1)){
                    present80Encrypt(session_key,message_out);
                    CANMessageSet(CAN0_BASE, 2, &msg_obj_send, MSG_OBJ_TYPE_TX);
                    g_sent=1;

                    Delay_ms(500);
                }
            //} // end if - received
        } // end for - normal operation
        UARTprintf("Normal Operation Completed!\n");
	}// end infinite while
	//msg_obj_send, msg_obj_rec

} //end main()

/*
InitKeys(hardcoded,response,secret_key,public_key);

    // Generate the node's public key
    Status = ECCRYPTO_SUCCESS;
    Status = CompressedKeyGeneration(secret_key, public_key);
    if (Status != ECCRYPTO_SUCCESS) {
        UARTprintf("Failed Public Key Generation\n");
        return Status;
    }
}
*/

// Set global flags to 0
static void resetFlags(void){
    g_bErrFlag = 0;
    g_recComplete = 0;
    g_resSend = 0;
    g_sharedReceived = 0;
    g_normalOp = 0;
    g_waitFlag = 0;
    g_normalFlag = 0;
    g_received = 0;
    g_sent = 0;
}


static void Delay_ms(uint32_t ms){
    SysCtlDelay((ms*80000)/3); // Uses 3 clock cycles per internal loop. 80MHz clk
}

