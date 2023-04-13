/**
 * \file
 *
 * \brief SAM CAN basic Quick Start
 *
 * Copyright (c) 2015-2018 Microchip Technology Inc. and its subsidiaries.
 *
 * \asf_license_start
 *
 * \page License
 *
 * Subject to your compliance with these terms, you may use Microchip
 * software and any derivatives exclusively with Microchip products.
 * It is your responsibility to comply with third party license terms applicable
 * to your use of third party software (including open source software) that
 * may accompany Microchip software.
 *
 * THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS". NO WARRANTIES,
 * WHETHER EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE,
 * INCLUDING ANY IMPLIED WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY,
 * AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT WILL MICROCHIP BE
 * LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE, INCIDENTAL OR CONSEQUENTIAL
 * LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND WHATSOEVER RELATED TO THE
 * SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP HAS BEEN ADVISED OF THE
 * POSSIBILITY OR THE DAMAGES ARE FORESEEABLE.  TO THE FULLEST EXTENT
 * ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL CLAIMS IN ANY WAY
 * RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT OF FEES, IF ANY,
 * THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS SOFTWARE.
 *
 * \asf_license_stop
 *
 */
/*
 * Support and FAQ: visit <a href="https://www.microchip.com/support/">Microchip Support</a>
 */
#include <asf.h>
#include <string.h>
#include <conf_can.h>
#include "FourQ/FourQ_api.h"
#include "Photon/photon.h"
#include "Enrollment.h"
//#include "Present/present.h"
#include "ASCON/api.h"
#include "ASCON/ascon.h"
#include "ASCON/crypto_aead.h"

//! [module_var]

//! [module_inst]
static struct usart_module cdc_instance;
static struct can_module can_instance;
//! [module_inst]


//! [can_filter_setting]
#define CAN_RX_STANDARD_FILTER_INDEX_0    0
#define CAN_RX_STANDARD_FILTER_INDEX_1    1
#define CAN_RX_STANDARD_FILTER_ID_0     0x45A
#define CAN_RX_STANDARD_FILTER_ID_0_BUFFER_INDEX     2
#define CAN_RX_STANDARD_FILTER_ID_1     0x469
#define CAN_RX_EXTENDED_FILTER_INDEX_0    0
#define CAN_RX_EXTENDED_FILTER_INDEX_1    1
#define CAN_RX_EXTENDED_FILTER_ID_0     0x100000A5
#define CAN_RX_EXTENDED_FILTER_ID_0_BUFFER_INDEX     1
#define CAN_RX_EXTENDED_FILTER_ID_1     0x10000096
//! [can_filter_setting]

#define CAN_FILTER_MONITOR 1
#define CAN_FILTER_WAIT 2
#define CAN_FILTER_MSG1 3
#define CAN_FILTER_MSG2 4

// [ENCRYPTION STUFF]
#define DEBUGCODE	0
#define NODE_ID		1
#define NODE_TOTAL	2
#define TOTAL_SENDS	10

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
//static volatile uint32_t g_sent = 0;

//! [ENCRYPTION STUFF]

//! [can_transfer_message_setting]
#define CAN_TX_BUFFER_INDEX    0
static uint8_t tx_message_0[CONF_CAN_ELEMENT_DATA_SIZE];
static uint8_t tx_message_1[CONF_CAN_ELEMENT_DATA_SIZE];
//! [can_transfer_message_setting]

//! [can_receive_message_setting]
static volatile uint32_t standard_receive_index = 0;
//static volatile uint32_t extended_receive_index = 0;
static struct can_rx_element_fifo_0 rx_element_fifo_0;
//static struct can_rx_element_fifo_1 rx_element_fifo_1;
//static struct can_rx_element_buffer rx_element_buffer;

//extern struct can_rx_element_buffer rx_element_buff[CONF_CAN0_RX_BUFFER_NUM];

//! [can_receive_message_setting]

//! [module_var]

//! [setup]
static void resetFlags(void);
static void Delay_ms(uint32_t delay);

//! [cdc_setup]
static void configure_usart_cdc(void)
{

	struct usart_config config_cdc;
	usart_get_config_defaults(&config_cdc);
	config_cdc.baudrate	 = 115200;
	config_cdc.mux_setting = EDBG_CDC_SERCOM_MUX_SETTING;
	config_cdc.pinmux_pad0 = EDBG_CDC_SERCOM_PINMUX_PAD0;
	config_cdc.pinmux_pad1 = EDBG_CDC_SERCOM_PINMUX_PAD1;
	config_cdc.pinmux_pad2 = EDBG_CDC_SERCOM_PINMUX_PAD2;
	config_cdc.pinmux_pad3 = EDBG_CDC_SERCOM_PINMUX_PAD3;
	stdio_serial_init(&cdc_instance, EDBG_CDC_MODULE, &config_cdc);
	usart_enable(&cdc_instance);
}
//! [cdc_setup]

//! [can_init_setup]
static void configure_can(void)
{
	uint32_t i;
	/* Initialize the memory. */
	for (i = 0; i < CONF_CAN_ELEMENT_DATA_SIZE; i++) {
		tx_message_0[i] = i;
		tx_message_1[i] = i + 0x80;
	}

	/* Set up the CAN TX/RX pins */
	struct system_pinmux_config pin_config;
	system_pinmux_get_config_defaults(&pin_config);
	pin_config.mux_position = CAN_TX_MUX_SETTING;
	system_pinmux_pin_set_config(CAN_TX_PIN, &pin_config);
	pin_config.mux_position = CAN_RX_MUX_SETTING;
	system_pinmux_pin_set_config(CAN_RX_PIN, &pin_config);

	/* Initialize the module. */
	struct can_config config_can;
	can_get_config_defaults(&config_can);
	can_init(&can_instance, CAN_MODULE, &config_can);

	can_enable_fd_mode(&can_instance);
	can_start(&can_instance);

	/* Enable interrupts for this CAN module */
	system_interrupt_enable(SYSTEM_INTERRUPT_MODULE_CAN0);
	can_enable_interrupt(&can_instance, CAN_PROTOCOL_ERROR_ARBITRATION
	| CAN_PROTOCOL_ERROR_DATA);
}
//! [can_init_setup]

void CAN0_Handler(void) {
volatile uint32_t rx_buffer_index;
volatile uint32_t status = can_read_interrupt_status(&can_instance);

//printf("Status = %i",status);

if ((status & CAN_PROTOCOL_ERROR_ARBITRATION)
|| (status & CAN_PROTOCOL_ERROR_DATA)) {
	can_clear_interrupt_status(&can_instance, CAN_PROTOCOL_ERROR_ARBITRATION
		| CAN_PROTOCOL_ERROR_DATA);
	printf("Protocol error, please double check the clock in two boards. \r\n\r\n");
}

// Enrollment stage:
if (STAGE == ENROLLMENT) {
	if (status & CAN_RX_BUFFER_NEW_MESSAGE) {
		can_clear_interrupt_status(&can_instance,CAN_RX_BUFFER_NEW_MESSAGE);
		for (int i = 0; i < CONF_CAN0_RX_BUFFER_NUM; i++) {
			if (can_rx_get_buffer_status(&can_instance, i)) {
				rx_buffer_index = i;
				can_rx_clear_buffer_status(&can_instance, i);
				int temp_Buff = rx_element_buff[rx_buffer_index].last_write;
				can_get_rx_buffer_element(&can_instance, &(rx_element_buff[rx_buffer_index].buffers[temp_Buff]),
				rx_buffer_index);
				rx_element_buff[rx_buffer_index].last_write = (temp_Buff + 1) % MAX_BUFFS;
				
				
				//if (rx_element_buff[rx_buffer_index].buffers[temp_Buff].R0.bit.XTD) {
				//	printf("\n\r Extended message received in Rx buffer. The received data is: \r\n");
				//	} else {
				//	printf("\n\r Standard message received in Rx buffer %d, section %d. The received data is: \r\n",rx_buffer_index,temp_Buff);
				//}
				//for (i = 0; i < rx_element_buff[rx_buffer_index].buffers[temp_Buff].R1.bit.DLC; i++) {
				//	printf("  %d",rx_element_buff[rx_buffer_index].buffers[temp_Buff].data[i]);
				//}
				//printf("\r\n\r\n");
				
				
				// Change flags based on buffer index:
				if (rx_buffer_index == CAN_FILTER_ENROLLMENT) {
					//printf("%d == %d ?\r\n",rx_buffer_index,CAN_FILTER_ENROLLMENT);
					g_rec = 1;
				}
					// Received message about type of communication
				
				else if (rx_buffer_index == CAN_FILTER_PUBLICKEY) {
					//printf("%d == %d ? also %d \r\n",rx_buffer_index,CAN_FILTER_PUBLICKEY,g_rec_public);
					g_rec_public++;
				} 
					// Received part of public key
			}
		}
	}
}

// Authentication Stage:
else if (STAGE == AUTHENTICATION) {
	if (status & CAN_RX_BUFFER_NEW_MESSAGE) {
		can_clear_interrupt_status(&can_instance,CAN_RX_BUFFER_NEW_MESSAGE);
		for (int i = 0; i < CONF_CAN0_RX_BUFFER_NUM; i++) {
			if (can_rx_get_buffer_status(&can_instance, i)) {
				rx_buffer_index = i;
				can_rx_clear_buffer_status(&can_instance, i);
				int temp_Buff = rx_element_buff[rx_buffer_index].last_write;
				can_get_rx_buffer_element(&can_instance, &(rx_element_buff[rx_buffer_index].buffers[temp_Buff]),
				rx_buffer_index);
				rx_element_buff[rx_buffer_index].last_write = (temp_Buff + 1) % MAX_BUFFS;
				
				
// 				if (rx_element_buff[rx_buffer_index].buffers[temp_Buff].R0.bit.XTD) {
// 					printf("\n\r Extended message received in Rx buffer. The received data is: \r\n");
// 					} else {
// 					printf("\n\r Standard message received in Rx buffer %d, section %d. The received data is: \r\n",rx_buffer_index,temp_Buff);
// 				}
// 				for (i = 0; i < rx_element_buff[rx_buffer_index].buffers[temp_Buff].R1.bit.DLC; i++) {
// 					printf("  %d",rx_element_buff[rx_buffer_index].buffers[temp_Buff].data[i]);
// 				}
// 				printf("\r\n\r\n");
				
				
				// Change flags based on buffer index:
				
				if (rx_buffer_index == CAN_FILTER_WAIT) g_waitFlag++;
				// Received Wait packet.
				
				else if (rx_buffer_index == CAN_FILTER_MONITOR) g_normalFlag++;
				// Received Monitor Packet
				
				else if (rx_buffer_index == CAN_FILTER_MSG2)  {
					if(g_sharedReceived == 0) {
						g_sharedReceived++;
					}
					else if ((g_sharedReceived == 1) && (g_recComplete == 1)) {
						g_sharedReceived++;
					}
				}
			}
		}
	}
	
	if (status & CAN_RX_FIFO_0_NEW_MESSAGE) {
		can_clear_interrupt_status(&can_instance, CAN_RX_FIFO_0_NEW_MESSAGE);
		can_get_rx_fifo_0_element(&can_instance, &rx_element_fifo_0,
		standard_receive_index);
		can_rx_fifo_acknowledge(&can_instance, 0,
		standard_receive_index);
		standard_receive_index++;
		if (standard_receive_index == CONF_CAN0_RX_FIFO_0_NUM) {
			standard_receive_index = 0;
		}
		
		
// 		printf("\n\r Standard message received in FIFO 0. The received data is: \r\n");
// 		for (int i = 0; i < rx_element_fifo_0.R1.bit.DLC; i++) {
// 			printf("  %d",rx_element_fifo_0.data[i]);
// 		}
// 		printf("\r\n\r\n");
		
		
		// Do thing here
		g_waitFlag++;
		// Received Wait packet.
		
	}
}
else {
	if (status & CAN_RX_FIFO_0_NEW_MESSAGE) {
		can_clear_interrupt_status(&can_instance, CAN_RX_FIFO_0_NEW_MESSAGE);
		can_get_rx_fifo_0_element(&can_instance, &rx_element_fifo_0,
		standard_receive_index);
		can_rx_fifo_acknowledge(&can_instance, 0,
		standard_receive_index);
		standard_receive_index++;
		if (standard_receive_index == CONF_CAN0_RX_FIFO_0_NUM) {
			standard_receive_index = 0;
		}

// 		printf("\n\r Standard message received in FIFO 0. The received data is: \r\n");
// 		for (int i = 0; i < rx_element_fifo_0.R1.bit.DLC; i++) {
// 			printf("  %d",rx_element_fifo_0.data[i]);
// 		}
// 		printf("\r\n\r\n");
		
		// Do thing here
		g_received++;
		//g_waitFlag++;
	}
}
}

//! [user_menu]
static void display_menu(void)
{
	printf("Menu :\r\n"
			"  -- Select the action:\r\n"
			"  0: Set standard filter ID 0: 0x45A, store into Rx buffer. \r\n"
			"  1: Set standard filter ID 1: 0x469, store into Rx FIFO 0. \r\n"
			"  2: Send standard message with ID: 0x45A and 4 byte data 0 to 3. \r\n"
			"  3: Send standard message with ID: 0x469 and 4 byte data 128 to 131. \r\n"
			"  4: Set extended filter ID 0: 0x100000A5, store into Rx buffer. \r\n"
			"  5: Set extended filter ID 1: 0x10000096, store into Rx FIFO 1. \r\n"
			"  6: Send extended message with ID: 0x100000A5 and 8 byte data 0 to 7. \r\n"
			"  7: Send extended message with ID: 0x10000096 and 8 byte data 128 to 135. \r\n"
			"  h: Display menu \r\n\r\n");
}
//! [user_menu]
void configure_rtc_count(void);

struct rtc_module rtc_instance;

void configure_rtc_count(void)
{
	//! [set_conf]
	struct rtc_count_config config_rtc_count;
	//! [set_conf]

	//! [get_default]
	rtc_count_get_config_defaults(&config_rtc_count);
	//! [get_default]

	//! [set_config]
	//config_rtc_count.prescaler           = RTC_COUNT_PRESCALER_DIV_1
	//config_rtc_count.mode                = RTC_COUNT_MODE_16BIT;
	#ifdef FEATURE_RTC_CONTINUOUSLY_UPDATED
	config_rtc_count.continuously_update = true;
	#endif
	//config_rtc_count.compare_values[0]   = 1000;
	//! [set_config]
	//! [init_rtc]
	rtc_count_init(&rtc_instance, RTC, &config_rtc_count);
	//! [init_rtc]

	//! [enable]
	rtc_count_enable(&rtc_instance);
	//! [enable]
}
//! [initiate]

//uint32_t ul_tickcount = 0;

//! [setup]
void SysTick_Handler(void){
	
	ul_tickcount += 300;
}

#define TIMEVAL (ul_tickcount+(14400000UL-1-SysTick->VAL)/48000UL)

int main(void)
{
	//uint8_t key;

//! [setup_init]
	system_init();
	configure_usart_cdc();
//! [setup_init]
	SysTick->CTRL = 0;					// Disable SysTick
	SysTick->LOAD = 14400000UL-1;			// Set reload register for 1mS interrupts
	NVIC_SetPriority(SysTick_IRQn, 3);	// Set interrupt priority to least urgency
	SysTick->VAL = 0;					// Reset the SysTick counter value
	SysTick->CTRL = 0x00000007;			// Enable SysTick, Enable SysTick Exceptions, Use CPU Clock
	NVIC_EnableIRQ(SysTick_IRQn);		// Enable SysTick Interrupt
	
	//configure_rtc_count();
	//rtc_count_set_period(&rtc_instance, 2000);
//! [main_setup]
	int last_num = 0;
//! [configure_can]
	configure_can();
//! [configure_can]

//! [display_user_menu]
	//display_menu();
	printf("Starting.\r\n");
//! [display_user_menu]
//volatile ECCRYPTO_STATUS status;
uint8_t secret_key[32];
uint8_t public_key[32];
uint8_t shared_secret[32];
uint8_t ServerPublicKey[32];
uint8_t response[16];
uint8_t ec[16];
uint8_t initData[24];
uint8_t * session_key = initData;
unsigned long mlen;

//uint8_t message[16];
uint8_t message_in[24];
uint8_t message_out[24];
uint8_t response_hash[16];
uint8_t shared_hash[16];
uint8_t encrypted_response_hash[32];
uint8_t server_reply[32];
ECCRYPTO_STATUS Status;

//printf("Time for enrollment\r\n");
bool hardcoded = (bool)Enrollment(NODE_ID, secret_key, ServerPublicKey, ec,&can_instance);

printf("Enrollment complete time: %d\r\n",TIMEVAL-startVal);

while(1) {
	startVal = TIMEVAL;
	resetFlags();
	
	// Set CAN to use Authentication Handler
	STAGE = AUTHENTICATION;
	
	//printf("Initialization Complete...\r\n");
	//printf("Trying to test this THANG...\r\n");
	
	// Generate the node's response and keys
	// InitKeys(hardcoded,response,secret_key,public_key);
	memset(secret_key+16,0,16);
	
	if(hardcoded) {
		//printf("Hardcoding Response\r\n");
		// Set each byte of node response to NODE_ID
		memset(response,NODE_ID,16);
	}
	else {
		printf("Generating Response\r\n");
		// Generate response
		// PUF Stuff
		if (NODE_ID == 1) {
			//Delay_ms(0);
		}
	}
	uint32_t tempVal = TIMEVAL;
	memmove(secret_key,response,16);
	
	// Hash the response to send to the server
	// secret_key's lowest 16 bytes contain the PUF's response
	photon128(response,16,response_hash);
	
	if (DEBUGCODE) {
		int j;
		
		printf("Hash is 0x");
		for(j = 15; j >= 0; j--) {
			printf("%02x",response_hash[j]);
		}
		printf("\r\n");
		
	}
	
	// Generate the node's public key
	Status = ECCRYPTO_SUCCESS;
	//printf("Secret is 0x");
	//for(int j = 15; j >= 0; j--) {
	//	printf("%02x",secret_key[j]);
	//}
	//printf("\r\n");
	Status = CompressedKeyGeneration(secret_key,public_key);
	if (Status != ECCRYPTO_SUCCESS) {
		printf("Failed Public Key Generation\r\n");
		return Status;
	}
	//printf("Public is 0x");
	//for(int j = 15; j >= 0; j--) {
	//	printf("%02x",public_key[j]);
	//}
	//printf("\r\n");
	
	
	
	// Generate Shared secret
	Status = CompressedSecretAgreement(secret_key,ServerPublicKey,shared_secret);
	if (Status != ECCRYPTO_SUCCESS) {
		printf("Failed Shared Secret Creation\r\n");
		return Status;
	}
	
	if (DEBUGCODE) {
		printf("Shared Secret: 0x");
		for (int i = 0; i < sizeof(shared_secret); i++) {
			printf("%02x",shared_secret[i]);
		}
		printf("\r\n");
		
		printf("Public Key: 0x");
		for (int i = 0; i < sizeof(public_key); i++) {
			printf("%02x",public_key[i]);
		}
		printf("\r\n");
		
		printf("Public Key: 0x");
		for (int i = 0; i < sizeof(public_key); i++) {
			printf("%02x",public_key[i]);
		}
		printf("\r\n");
	}
	
	struct can_standard_message_filter_element sd_filter;
	struct can_tx_element tx_element;
	
	// Receive
	can_get_standard_message_filter_element_default(&sd_filter);
	sd_filter.S0.bit.SFID1 = 0x0;
	sd_filter.S0.bit.SFID2 = 0x0;

	can_set_rx_standard_filter(&can_instance, &sd_filter,
		CAN_FILTER_WAIT);
	can_enable_interrupt(&can_instance, CAN_RX_FIFO_0_NEW_MESSAGE);
	
	// Enable monitoring
	can_get_standard_message_filter_element_default(&sd_filter);
	sd_filter.S0.bit.SFID1 = 0x301;
	sd_filter.S0.bit.SFID2 = CAN_FILTER_MONITOR;
	sd_filter.S0.bit.SFEC =
	CAN_STANDARD_MESSAGE_FILTER_ELEMENT_S0_SFEC_STRXBUF_Val;
	can_set_rx_standard_filter(&can_instance, &sd_filter,
	CAN_FILTER_MONITOR);
	can_enable_interrupt(&can_instance, CAN_RX_BUFFER_NEW_MESSAGE);
	
	// Hash 32-byte shared secret to 128 bits then truncate to 10 bytes
	// Only first 10 bytes of shared_hash will be used
	photon128(shared_secret,32,shared_hash);
	
	// Encrypt the two halves of the response_hash (8 bytes at a time)
	// The first 10 bytes of shared_hash is the key
	//memmove(encrypted_response_hash,response_hash,16);
	unsigned long clen = 16;
	printf("Hash Encrypt: Input to tx_element size: %04x\r\n",clen);
	crypto_aead_encrypt(encrypted_response_hash, &clen, response_hash, 16, NULL, NULL, NULL, n, shared_hash);
	printf("Hash Encrypt: Output to tx_element size: %04x\r\n",clen);
	//present80Encrypt(shared_hash,encrypted_response_hash);
	//present80Encrypt(shared_hash,&encrypted_response_hash[8]);
	
	// Wait your turn
	// TODO: Change to the FIFO setup
	
	//printf("Waiting\r\n");
	printf("TEMP TIME: %d\r\n",TIMEVAL-tempVal);
	// Prevents warning from 1st node comparing unsigned int with 0
#if NODE_ID > 1
	while(g_waitFlag < (NODE_ID-1))
	//printf("g_waitFlag is now %i",g_waitFlag);
		for(int i = 0; i < 100000; i++);
//#else
	// Node 1 needs to give server enough time to generate all shared secrets
	//if (DEBUGCODE) {
		// There is a lot for the server to print
		//Delay_ms(NODE_TOTAL * 1);
	//}
	//else {
		//Delay_ms(NODE_TOTAL * 1);
	//}
#endif
	
	// It's your turn so stop listening
	//sd_filter.S0.bit.SFEC =
	//	CAN_STANDARD_MESSAGE_FILTER_ELEMENT_S0_SFEC_DISABLE_Val;
	//can_set_rx_standard_filter(&can_instance, &sd_filter,
	//	CAN_FILTER_WAIT);
	//can_disable_interrupt(&can_instance, CAN_RX_FIFO_0_NEW_MESSAGE);
	printf("My Turn!\r\n");
	
	// Send to the server
	// Set to send CAN0 msg 1;
	//Delay_ms(50);
	can_get_tx_buffer_element_defaults(&tx_element);
	tx_element.T0.reg |= CAN_TX_ELEMENT_T0_STANDARD_ID(0x100+NODE_ID);
	tx_element.T1.reg = CAN_TX_ELEMENT_T1_FDF | CAN_TX_ELEMENT_T1_BRS |
		CAN_TX_ELEMENT_T1_DLC(CAN_TX_ELEMENT_T1_DLC_DATA16_Val);
	memcpy(tx_element.data,encrypted_response_hash,16);
	can_set_tx_buffer_element(&can_instance, &tx_element,
		CAN_TX_FILTER_BUFFER_INDEX);
	can_tx_transfer_request(&can_instance, 1 << CAN_TX_FILTER_BUFFER_INDEX);
	
	while(!(can_tx_get_transmission_status(&can_instance) & (1 << CAN_TX_FILTER_BUFFER_INDEX)));
	//printf("First and Second message sent!\r\n");
	//Potentially needed: Delay_ms(50);
	
	 // Send second half
	 /*
	can_get_tx_buffer_element_defaults(&tx_element);
	tx_element.T0.reg |= CAN_TX_ELEMENT_T0_STANDARD_ID(0x100+NODE_ID);
	tx_element.T1.bit.DLC = 8;
	memcpy(tx_element.data,&encrypted_response_hash[8],8);
	can_set_tx_buffer_element(&can_instance, &tx_element,
	CAN_TX_FILTER_BUFFER_INDEX);
	can_tx_transfer_request(&can_instance, 1 << CAN_TX_FILTER_BUFFER_INDEX);
	
	while(!(can_tx_get_transmission_status(&can_instance) & (1 << CAN_TX_FILTER_BUFFER_INDEX)));
	printf("Second message sent!\r\n");
	*/
	g_waitFlag += 1;
	
	// Wait for the other nodes to send their hashed responses
	//can_get_standard_message_filter_element_default(&sd_filter);
	//sd_filter.S0.bit.SFID1 = 0x0;
	//sd_filter.S0.bit.SFID2 = 0x0;

	//can_set_rx_standard_filter(&can_instance, &sd_filter,
	//	CAN_FILTER_WAIT);
	//can_enable_interrupt(&can_instance, CAN_RX_FIFO_0_NEW_MESSAGE);
	
	can_get_standard_message_filter_element_default(&sd_filter);
	sd_filter.S0.bit.SFID1 = 0x200 + NODE_ID;
	sd_filter.S0.bit.SFID2 = CAN_FILTER_MSG2;
	sd_filter.S0.bit.SFEC =
	CAN_STANDARD_MESSAGE_FILTER_ELEMENT_S0_SFEC_STRXBUF_Val;
	can_set_rx_standard_filter(&can_instance, &sd_filter,
		CAN_FILTER_MSG2);
	//printf("Waiting for others\r\n");
	while(g_waitFlag < (NODE_TOTAL));
	
	printf("After \"Waiting for others\"\r\n");
	sd_filter.S0.bit.SFEC =
		CAN_STANDARD_MESSAGE_FILTER_ELEMENT_S0_SFEC_DISABLE_Val;
	can_set_rx_standard_filter(&can_instance, &sd_filter,
		CAN_FILTER_WAIT);
	//printf("All nodes finished sending hashed responses\r\n");
	
	// Set CAN0 message to receive.
// 	can_get_standard_message_filter_element_default(&sd_filter);
// 	sd_filter.S0.bit.SFID1 = 0x200 + NODE_ID;
// 	sd_filter.S0.bit.SFID2 = CAN_FILTER_MSG2;
// 	sd_filter.S0.bit.SFEC =
// 		CAN_STANDARD_MESSAGE_FILTER_ELEMENT_S0_SFEC_STRXBUF_Val;
// 	can_set_rx_standard_filter(&can_instance, &sd_filter,
// 		CAN_FILTER_MSG2);
	
	// Wait for server response containing shared secret
	while(g_sharedReceived==0);
	memcpy(server_reply,getNextBufferElement(&rx_element_buff[CAN_FILTER_MSG2])->data,24);
	g_recComplete++;
	//printf("Received first message\r\n");
	
	// Wait for second half of server response to arrive
	/*
	while(g_sharedReceived==1);
	printf("Received second message\r\n");
	memcpy(server_reply,getNextBufferElement(&rx_element_buff[CAN_FILTER_MSG2])->data,8);
	*/
	
	//printf("Msg ID=0x%04x received: ",(uint32_t)(getNextBufferElement(&rx_element_buff[CAN_FILTER_MSG2])->R0.bit.ID));
	//for(int i=0;i<16;i++){printf("%02x ",server_reply[i]);}
	//printf("\r\n");
	
	crypto_aead_decrypt(initData, &mlen, (void*)0, server_reply, 24, NULL, NULL, n, shared_hash);
	printf("Session key Decrypt: Output to message_in size: %i\r\n",mlen);
	//memcpy(n,&initData[24],8);
	printf("SessionKey:");
	for(int i = 0; i < 16; i++) {
		printf("%x ",session_key[i]);
	}
	printf("\r\n");
	//present80Decrypt(shared_hash,server_reply);
	//present80Decrypt(shared_hash,&server_reply[8]);
	
	//memmove(session_key,server_reply,10);
	
	//printf("Finished Decrypting!\r\n");
	//printf("Sessions key is: 0x");
	//for(int i=9;i>=0;i--){printf("%02x",session_key[i]);}
	// Receive on 1
	can_get_standard_message_filter_element_default(&sd_filter);
	sd_filter.S0.bit.SFID1 = 0x400 + NODE_ID;
	sd_filter.S0.bit.SFID2 = 0x7F0;

	can_set_rx_standard_filter(&can_instance, &sd_filter,
	CAN_RX_STANDARD_FILTER_INDEX_1);
	can_enable_interrupt(&can_instance, CAN_RX_FIFO_0_NEW_MESSAGE);
	
	// Set up for normal communication with other ECUs
	while(g_normalFlag == 0);
	STAGE = NORMAL;
	
	sd_filter.S0.bit.SFEC =
		CAN_STANDARD_MESSAGE_FILTER_ELEMENT_S0_SFEC_DISABLE_Val;
	can_set_rx_standard_filter(&can_instance, &sd_filter,
		CAN_FILTER_MSG2);
	//printf("\r\nNormal Operation Time\r\n");
	
	// Send on 2
	can_get_tx_buffer_element_defaults(&tx_element);
	tx_element.T0.reg |= CAN_TX_ELEMENT_T0_STANDARD_ID(0x400+NODE_ID);
	tx_element.T1.reg = CAN_TX_ELEMENT_T1_FDF | CAN_TX_ELEMENT_T1_BRS |
		CAN_TX_ELEMENT_T1_DLC(CAN_TX_ELEMENT_T1_DLC_DATA24_Val);
	memset(message_out,0,16);
	//Delay_ms(10);
	//memcpy(tx_element.data,&encrypted_response_hash[8],8);
	//can_set_tx_buffer_element(&can_instance, &tx_element,
	//	CAN_TX_FILTER_BUFFER_INDEX);
	//can_tx_transfer_request(&can_instance, 1 << CAN_TX_FILTER_BUFFER_INDEX);
	g_sent = 0;
	
	uint32_t endVal = TIMEVAL;
	printf("End: %d\r\n",endVal);
	printf("Difference: %d\r\n",endVal-startVal);
	g_waitFlag = 0;
	
	if (NODE_ID == 1) {
		//present80Encrypt(session_key,message_out);
		//printf("Raw Send: 0x%08x.%08x\r\n",*((uint32_t *)&message_out[4]),*((uint32_t *)message_out));
		crypto_aead_encrypt(tx_element.data, &clen, message_out, 16, NULL, NULL, NULL, n, session_key);
		//printf("Encrypt as Node 1: Output to tx_element size: %i\r\n",clen);
		
		//memcpy(tx_element.data,message_out,24);
		can_set_tx_buffer_element(&can_instance, &tx_element,
			CAN_TX_FILTER_BUFFER_INDEX);
		can_tx_transfer_request(&can_instance, 1 << CAN_TX_FILTER_BUFFER_INDEX);
		g_sent = 1;
		g_waitFlag++;
		//Delay_ms(20);
	}
	
	// Send and received a fix number of messages until restarting
	// With the authentication phase
	volatile uint32_t timeOut = TIMEVAL;
	for (int i = 0; i < TOTAL_SENDS; i++) {
		while(g_sent != 0) {
			if ((can_tx_get_transmission_status(&can_instance) & (1 << CAN_TX_FILTER_BUFFER_INDEX))) {
				g_sent = 0;
			}
		}
		
		//while(!(can_tx_get_transmission_status(can_inst) & (1 << CAN_TX_FILTER_BUFFER_INDEX)));
		while(g_received == 0);
		uint32_t recTime = TIMEVAL;
		g_received = 0;
		//memcpy(message_in,rx_element_fifo_0.data,24);
		//present80Decrypt(session_key,message_in);
		//printf("Raw Receive: 0x%08x.%08x\r\n",*((uint32_t *)&rx_element_fifo_0.data[4]),*((uint32_t *)rx_element_fifo_0.data));
		crypto_aead_decrypt(message_in, &mlen, (void*)0, rx_element_fifo_0.data, 24, NULL, NULL, n, session_key);
		//printf("Decrypt: Output to message_in size: %i\r\n",mlen);
		printf("Received: 0x%08x.%08x\r\n",*((uint32_t *)&message_in[4]),*((uint32_t *)message_in));
		*((uint32_t *)message_in) += 1;
		memmove(message_out,message_in,16);
		printf("///////////////\r\nTime to receive: %d\r\n",TIMEVAL-recTime);
		//printf("g_waitflag: %i\r\n",g_waitFlag);
		
		if ((i<TOTAL_SENDS-1) || (NODE_ID != 1)) {
			timeOut = TIMEVAL;	
			//printf("TIMEVALLLLLLLLLLLLLLLLLLL: %i\r\n",TIMEVAL);
			//printf("Raw Send: 0x%08x.%08x\r\n",*((uint32_t *)&message_out[4]),*((uint32_t *)message_out));
			crypto_aead_encrypt(tx_element.data, &clen, message_out, 16, NULL, NULL, NULL, n, session_key);
			//printf("Encrypt: Output to tx_element size: %i\r\n",clen);
			//memcpy(tx_element.data,message_out,8);
			can_set_tx_buffer_element(&can_instance, &tx_element,
				CAN_TX_FILTER_BUFFER_INDEX);
			can_tx_transfer_request(&can_instance, 1 << CAN_TX_FILTER_BUFFER_INDEX);
			while(!(can_tx_get_transmission_status(&can_instance) & (1 << CAN_TX_FILTER_BUFFER_INDEX)));
			g_sent = 0;
			g_waitFlag++;
			printf("///////////////\r\nTime to send: %d\r\n",TIMEVAL-timeOut);
			//Delay_ms(5);
		}
	}
	printf("Normal Operation Completed!\r\n");
	
}

//! [main_loop]
/*
	while(1) {
		scanf("%c", (char *)&key);

		switch (key) {
		case 'h':
			display_menu();
			break;
		
		case 'b':
			printf("  Set filters 4 = 0x69, 5 = 0x420. \r\n");
			can_set_Rx_Buffer_filter(0x69,0x4,0x2,&can_instance);
			can_set_Rx_Buffer_filter(0x420,0x5,0x3,&can_instance);
			break;
		
		case 'n':
			printf("  2: Send standard message with ID: 0x45A and 8 byte data 0 to 7. \r\n");
			can_send_standard_message(0x69, tx_message_0,
			CONF_CAN_ELEMENT_DATA_SIZE);
			break;
		
		case 'c':
			printf("  2: Send standard message with ID: 0x45A and 8 byte data 0 to 7. \r\n");
			can_send_standard_message(0x420, tx_message_1,
			CONF_CAN_ELEMENT_DATA_SIZE);
			break;
		
		case 'o':
			printf("Buff 4: ");
			for (int i = 0; i < rx_element_buff[4].R1.bit.DLC; i++) {
				printf("  %d",rx_element_buff[4].data[i]);
			}
			printf("\r\n");
			printf("Buff 5: ");
			for (int i = 0; i < rx_element_buff[5].R1.bit.DLC; i++) {
				printf("  %d",rx_element_buff[5].data[i]);
			}
			printf("\r\n");
			break;
			
		case '0':
			printf("  0: Set standard filter ID 0: 0x45A, store into Rx buffer. \r\n");
			can_set_standard_filter_0();
			break;

		case '1':
			printf("  1: Set standard filter ID 1: 0x469, store into Rx FIFO 0. \r\n");
			can_set_standard_filter_1();
			break;

		case '2':
			printf("  2: Send standard message with ID: 0x45A and 8 byte data 0 to 7. \r\n");
			can_send_standard_message(CAN_RX_STANDARD_FILTER_ID_0, tx_message_0,
					CONF_CAN_ELEMENT_DATA_SIZE);
			break;

		case '3':
			printf("  3: Send standard message with ID: 0x469 and 4 byte data 128 to 131. \r\n");
			can_send_standard_message(CAN_RX_STANDARD_FILTER_ID_1, tx_message_1,
					CONF_CAN_ELEMENT_DATA_SIZE / 2);
			break;

		case '4':
			printf("  4: Set extended filter ID 0: 0x100000A5, store into Rx buffer. \r\n");
			can_set_extended_filter_0();
			break;

		case '5':
			printf("  5: Set extended filter ID 1: 0x10000096, store into Rx FIFO 1. \r\n");
			can_set_extended_filter_1();
			break;

		case '6':
			printf("  6: Send extended message with ID: 0x100000A5 and 8 byte data 0 to 7. \r\n");
			can_send_extended_message(CAN_RX_EXTENDED_FILTER_ID_0, tx_message_0,
					CONF_CAN_ELEMENT_DATA_SIZE);
			break;

		case '7':
			printf("  7: Send extended message with ID: 0x10000096 and 8 byte data 128 to 135. \r\n");
			can_send_extended_message(CAN_RX_EXTENDED_FILTER_ID_1, tx_message_1,
					CONF_CAN_ELEMENT_DATA_SIZE);
			break;

		default:
			break;
		}
	}
*/
//! [main_loop]

//! [main_setup]
}

static void resetFlags(void) {
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

static void Delay_ms(uint32_t Delay) {
	printf("Delayed here");
	for(uint32_t i = 0; i < Delay*10000; i++);
}