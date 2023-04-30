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
#include "Enrollment_Server.h"
#include "Present/present.h"
#include "ASCON/api.h"
#include "ASCON/ascon.h"
#include "ASCON/crypto_aead.h"

//! [module_var]

//! [module_inst]
static struct usart_module cdc_instance;
static struct can_module can0_instance;
static struct can_module can1_instance;
//! [module_inst]

// [ENCRYPTION STUFF]
#define DEBUGCODE	0
#define NODE_ID		0
#define NODE_TOTAL	2
#define TOTAL_SENDS	10
#define TOTAL_MESSAGES (NODE_TOTAL*TOTAL_SENDS)

// General error flag
static volatile uint32_t g_bErrFlag=0;

// Make sure both halves can be received
static volatile uint32_t g_hash_res[NODE_TOTAL+1];

// Make sure 1st half of session key has been sent before sending the next one
static volatile uint32_t g_session_res[NODE_TOTAL+1];

// Mask to store valid nodes. Updated as Nodes are authenticated.
static volatile uint32_t g_valid_nodes_mask = 1;

// Flags for displaying messages during normal operation
// (i.e. monitor all traffic on the bus)
static volatile uint32_t g_normal_received_mask = 0;
static volatile uint32_t g_normal_received_flags[NODE_TOTAL+1];

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
//extern volatile enum OPERATION STAGE;
//! [can_receive_message_setting]

// Extra spot to simplify look-up since nodes begin at 1
// Server's value is at 0 when applicable
uint8_t StoredPublicKeys[NODE_TOTAL+1][32];
uint8_t StoredResponseHashes[NODE_TOTAL+1][16];
uint8_t NodeSharedSecrets[NODE_TOTAL+1][16];
// Hashed shared secrets for each Node. PRESENT80 only needs bottom 10 bytes.
uint8_t ReceivedResponseHashes[NODE_TOTAL+1][16];
uint8_t EncryptedSessionKeys[NODE_TOTAL+1][16];

//! [module_var]

//! [setup]
bool InitKeys(bool hardcoded, uint8_t * secret_key, uint8_t *ServerPublicKey, uint8_t *ec);
bool InitSharedSecrets(uint8_t *private_key, uint8_t (*StoredPublicKeys)[32], uint8_t (*NodeSharedSecrets)[16]);
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
	
	// Set up the CAN1 TX/RX pin
	pin_config.mux_position = CAN_TX_MUX_SETTING_2;
	system_pinmux_pin_set_config(CAN_TX_PIN_2,&pin_config);
	pin_config.mux_position = CAN_RX_MUX_SETTING_2;
	system_pinmux_pin_set_config(CAN_RX_PIN_2,&pin_config);
	
	/* Initialize the module. */
	struct can_config config_can;
	can_get_config_defaults(&config_can);
	can_init(&can0_instance, CAN_MODULE, &config_can);
	can_init(&can1_instance, CAN_MODULE_2, &config_can);

	can_enable_fd_mode(&can0_instance);
	can_start(&can0_instance);
	
	can_enable_fd_mode(&can1_instance);
	can_start(&can1_instance);
	
	/* Enable interrupts for this CAN module */
	system_interrupt_enable(SYSTEM_INTERRUPT_MODULE_CAN0);
	system_interrupt_enable(SYSTEM_INTERRUPT_MODULE_CAN1);
	can_enable_interrupt(&can0_instance, CAN_PROTOCOL_ERROR_ARBITRATION
	| CAN_PROTOCOL_ERROR_DATA);
	can_enable_interrupt(&can1_instance, CAN_PROTOCOL_ERROR_ARBITRATION
	| CAN_PROTOCOL_ERROR_DATA);
}
//! [can_init_setup]

// TODO: completely rehaul the handler + add a Tx handle?
void CAN0_Handler(void) {
volatile uint32_t rx_buffer_index;
volatile uint32_t status = can_read_interrupt_status(&can0_instance);
//printf("Status = %i, Pubkey[2][6] = %02x",status,StoredPublicKeys[2][6]);

if ((status & CAN_PROTOCOL_ERROR_ARBITRATION)
|| (status & CAN_PROTOCOL_ERROR_DATA)) {
	can_clear_interrupt_status(&can0_instance, CAN_PROTOCOL_ERROR_ARBITRATION
		| CAN_PROTOCOL_ERROR_DATA);
	//printf("Protocol error, please double check the clock in two boards. \r\n\r\n");
}

// Enrollment stage:
else if (STAGE == ENROLLMENT) {
	if (status & CAN_RX_BUFFER_NEW_MESSAGE) {
		can_clear_interrupt_status(&can0_instance,CAN_RX_BUFFER_NEW_MESSAGE);
		for (int i = 0; i < CONF_CAN0_RX_BUFFER_NUM; i++) {
			if (can_rx_get_buffer_status(&can0_instance, i)) {
				rx_buffer_index = i;
				can_rx_clear_buffer_status(&can0_instance, i);
				int temp_Buff = rx_element_buff[rx_buffer_index].last_write;
				can_get_rx_buffer_element(&can0_instance, &(rx_element_buff[rx_buffer_index].buffers[temp_Buff]),
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
				if (rx_buffer_index == CAN_FILTER_REGULAR_REC) g_received++;
				
			}
		}
	}
}

// Receive Stage:
else if (STAGE == RECEIVE) {
	if (status & CAN_RX_BUFFER_NEW_MESSAGE) {
		can_clear_interrupt_status(&can0_instance,CAN_RX_BUFFER_NEW_MESSAGE);
		for (int i = 0; i < CONF_CAN0_RX_BUFFER_NUM; i++) {
			if (can_rx_get_buffer_status(&can0_instance, i)) {
				rx_buffer_index = i;
				can_rx_clear_buffer_status(&can0_instance, i);
				int temp_Buff = rx_element_buff[rx_buffer_index].last_write;
				can_get_rx_buffer_element(&can0_instance, &(rx_element_buff[rx_buffer_index].buffers[temp_Buff]),
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
				g_hash_res[rx_buffer_index]++;
			}
		}
	}
}

// Send stage
else if (STAGE == SEND) {
	if (status & CAN_RX_BUFFER_NEW_MESSAGE) {
		can_clear_interrupt_status(&can0_instance,CAN_RX_BUFFER_NEW_MESSAGE);
		for (int i = 0; i < CONF_CAN0_RX_BUFFER_NUM; i++) {
			if (can_rx_get_buffer_status(&can0_instance, i)) {
				rx_buffer_index = i;
				can_rx_clear_buffer_status(&can0_instance, i);
				int temp_Buff = rx_element_buff[rx_buffer_index].last_write;
				can_get_rx_buffer_element(&can0_instance, &(rx_element_buff[rx_buffer_index].buffers[temp_Buff]),
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
				//int concern_flag = 1;
				g_session_res[rx_buffer_index]++;
				g_session_res[0]++;
				break;
			}
		}
	}
}

// Normal
else {
	if (status & CAN_RX_BUFFER_NEW_MESSAGE) {
		can_clear_interrupt_status(&can0_instance,CAN_RX_BUFFER_NEW_MESSAGE);
		for (int i = 0; i < CONF_CAN0_RX_BUFFER_NUM; i++) {
			if (can_rx_get_buffer_status(&can0_instance, i)) {
				rx_buffer_index = i;
				can_rx_clear_buffer_status(&can0_instance, i);
				int temp_Buff = rx_element_buff[rx_buffer_index].last_write;
				can_get_rx_buffer_element(&can0_instance, &(rx_element_buff[rx_buffer_index].buffers[temp_Buff]),
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
				//int concern_flag = 1;
				g_normal_received_flags[rx_buffer_index]++;
				g_normal_received_mask |= (1<<rx_buffer_index);
				break;
			}
		}
	}
}

}

void CAN1_Handler(void) {
	volatile uint32_t rx_buffer_index;
	volatile uint32_t status = can_read_interrupt_status(&can1_instance);
	//printf("Status = %i, Pubkey[2][6] = %02x",status,StoredPublicKeys[2][6]);

	if ((status & CAN_PROTOCOL_ERROR_ARBITRATION)
	|| (status & CAN_PROTOCOL_ERROR_DATA)) {
		can_clear_interrupt_status(&can1_instance, CAN_PROTOCOL_ERROR_ARBITRATION
		| CAN_PROTOCOL_ERROR_DATA);
		//printf("Protocol error, please double check the clock in two boards. \r\n\r\n");
	}

	// Enrollment stage:
	else if (STAGE == ENROLLMENT) {
		if (status & CAN_RX_BUFFER_NEW_MESSAGE) {
			can_clear_interrupt_status(&can1_instance,CAN_RX_BUFFER_NEW_MESSAGE);
			for (int i = 0; i < CONF_CAN0_RX_BUFFER_NUM; i++) {
				if (can_rx_get_buffer_status(&can1_instance, i)) {
					rx_buffer_index = i;
					can_rx_clear_buffer_status(&can1_instance, i);
					int temp_Buff = rx_element_buff[rx_buffer_index].last_write;
					can_get_rx_buffer_element(&can1_instance, &(rx_element_buff[rx_buffer_index].buffers[temp_Buff]),
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
					if (rx_buffer_index == CAN_FILTER_REGULAR_REC) g_received++;
					
				}
			}
		}
	}

	// Receive Stage:
	else if (STAGE == RECEIVE) {
		if (status & CAN_RX_BUFFER_NEW_MESSAGE) {
			can_clear_interrupt_status(&can1_instance,CAN_RX_BUFFER_NEW_MESSAGE);
			for (int i = 0; i < CONF_CAN0_RX_BUFFER_NUM; i++) {
				if (can_rx_get_buffer_status(&can1_instance, i)) {
					rx_buffer_index = i;
					can_rx_clear_buffer_status(&can1_instance, i);
					int temp_Buff = rx_element_buff[rx_buffer_index].last_write;
					can_get_rx_buffer_element(&can1_instance, &(rx_element_buff[rx_buffer_index].buffers[temp_Buff]),
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
					g_hash_res[rx_buffer_index]++;
				}
			}
		}
	}

	// Send stage
	else if (STAGE == SEND) {
		if (status & CAN_RX_BUFFER_NEW_MESSAGE) {
			can_clear_interrupt_status(&can1_instance,CAN_RX_BUFFER_NEW_MESSAGE);
			for (int i = 0; i < CONF_CAN0_RX_BUFFER_NUM; i++) {
				if (can_rx_get_buffer_status(&can1_instance, i)) {
					rx_buffer_index = i;
					can_rx_clear_buffer_status(&can1_instance, i);
					int temp_Buff = rx_element_buff[rx_buffer_index].last_write;
					can_get_rx_buffer_element(&can1_instance, &(rx_element_buff[rx_buffer_index].buffers[temp_Buff]),
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
					//int concern_flag = 1;
					g_session_res[rx_buffer_index]++;
					g_session_res[0]++;
					break;
				}
			}
		}
	}

	// Normal
	else {
		if (status & CAN_RX_BUFFER_NEW_MESSAGE) {
			can_clear_interrupt_status(&can1_instance,CAN_RX_BUFFER_NEW_MESSAGE);
			for (int i = 0; i < CONF_CAN0_RX_BUFFER_NUM; i++) {
				if (can_rx_get_buffer_status(&can1_instance, i)) {
					rx_buffer_index = i;
					can_rx_clear_buffer_status(&can1_instance, i);
					int temp_Buff = rx_element_buff[rx_buffer_index].last_write;
					can_get_rx_buffer_element(&can1_instance, &(rx_element_buff[rx_buffer_index].buffers[temp_Buff]),
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
					//int concern_flag = 1;
					g_normal_received_flags[rx_buffer_index]++;
					g_normal_received_mask |= (1<<rx_buffer_index);
					break;
				}
			}
		}
	}

}


//! [user_menu]
/*static void display_menu(void)
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
//! [user_menu]*/

bool InitKeys(bool hardcoded, uint8_t * secret_key, uint8_t *ServerPublicKey, uint8_t *ec) {
	ECCRYPTO_STATUS Status;
	
	memset(secret_key+16,0,16);
	
	if(hardcoded){
		memset(secret_key,0xFF,16);
	}
	else {
		printf("No PUF available\r\n");
		while(1);
	}
	
	Status = ECCRYPTO_SUCCESS;
	Status = CompressedKeyGeneration(secret_key, ServerPublicKey);
	if (Status != ECCRYPTO_SUCCESS) {
		printf("Failed public key generation!\r\n");
		return false;
	}
	
	if(DEBUGCODE) {
		printf("DEBUG: ServerPublicKey: 0x");
		for (int i = 31; i >=0 ; i--) {
			printf("%02x",ServerPublicKey[i]);
		}
		printf("\r\n");
	}
	
	return true;
}

bool InitSharedSecrets(uint8_t *private_key, uint8_t (*StoredPublicKeys)[32], uint8_t (*NodeSharedSecrets)[16]) {
	ECCRYPTO_STATUS Status = ECCRYPTO_SUCCESS;
	uint8_t shared_secret[32];

	for(int i = 1; i <= NODE_TOTAL; i++) {
		// Generate Shared Secret
		if(DEBUGCODE) {
			printf("Node %i public key is 0x",i);
			for(int j = 31; j >= 0; j--) {
				printf("%02x ",StoredPublicKeys[i][j]);
			}
			printf("\r\n");
			
			printf("Private key is 0x");
			for(int i = 31; i>= 0; i--) {
				printf("%02x",private_key[i]);
			}
			printf("\r\n");
		}
		
		Status = CompressedSecretAgreement(private_key,StoredPublicKeys[i],shared_secret);
		
		if(DEBUGCODE) {
			printf("Shared Secret is 0x");
			for(int i = 31; i>= 0; i--) {
				printf("%02x",shared_secret[i]);
			}
			printf("\r\n");
		}
		
		if (Status != ECCRYPTO_SUCCESS) {
			printf("Failed node %i Shared Secret Generation\r\n",i);
			printf("Fail code: %i\r\n",Status);
			return false;
		}
		//else {
		//	printf("Node %i secret generation success!\r\n",i);
		//}
		
		photon128(shared_secret,32,NodeSharedSecrets[i]);
	}
	return true;
}
//! [setup]

//! [setup]
void SysTick_Handler(void){
	
	ul_tickcount += 10;
	
}

#define TIMEVAL (ul_tickcount+(48000000UL-1UL-SysTick->VAL)/48000UL)

//uint8_t input_key[10] = {0x03,0x57,0x21,0x25,0x85,0xa4,0xb3,0x4f,0x93,0x11};
//uint8_t datas[8] = {0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07};
	
uint8_t input_key[10] = {0x01,0x03,0x02,0x05,0x04,0x07,0x06,0x09,0x08,0x10};
uint8_t datas[8] = {0x1,0x2,0x3,0x4,0x5,0x6,0x7,0x8};
unsigned char n[CRYPTO_NPUBBYTES] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};
	
int main(void)
{
	//uint8_t key;
//! [setup_init]
	system_init();
	configure_usart_cdc();
//! [setup_init]

//! [main_setup]
//! [configure_can]
	configure_can();
//! [configure_can]

//! [display_user_menu]
	//display_menu();
	printf("Starting.\r\n");
	
//! [display_user_menu]

	SysTick->CTRL = 0;					// Disable SysTick
	SysTick->LOAD = 48000000UL-1;		// Set reload register for 1mS interrupts
	NVIC_SetPriority(SysTick_IRQn, 3);	// Set interrupt priority to least urgency
	SysTick->VAL = 0;					// Reset the SysTick counter value
	SysTick->CTRL = 0x00000007;			// Enable SysTick, Enable SysTick Exceptions, Use CPU Clock
	NVIC_EnableIRQ(SysTick_IRQn);		// Enable SysTick Interrupt
	
	//volatile ECCRYPTO_STATUS status;
	volatile uint8_t seed;
	bool check;
	bool hardcoded;
	uint8_t secret_key[32];
	uint8_t ec[16];
	unsigned long clen = 16;
	unsigned long mlen = 16;
	uint8_t message[NODE_TOTAL+1][8];
	uint8_t session_key[16];
	//struct can_standard_message_filter_element sd_filter;
	struct can_standard_message_filter_element res_filter[NODE_TOTAL+1];
	struct can_tx_element tx_element;
	struct can_tx_element send_filter[NODE_TOTAL+1];
	
	memset(rx_element_buff,0,CONF_CAN0_RX_BUFFER_NUM*sizeof(multiBuffer));
	printf("\r\nPress any key to enter a random key seed: ");
	scanf("%c", &seed);
	printf("\r\n");
	
	uint32_t startupTime = TIMEVAL;
	hardcoded = EnrollNodes(NODE_TOTAL,StoredPublicKeys,StoredResponseHashes,ec,&can0_instance,&can1_instance);
	printf("Completed enrollment phase with time: %d",TIMEVAL-startupTime);
	
	while(1) {
		startVal = TIMEVAL;
		// Need to change the handler stage
		STAGE = RECEIVE;
	
		//printf("Initialization Complete...\r\n");
		//printf("Testing the server...\r\n");
		
// 		printf("Randomly: Node %i public key is 0x",2);
// 			for(int j = 31; j >= 0; j--) {
// 				printf("%02x ",StoredPublicKeys[2][j]);
// 			}
// 		printf("\r\n");
		
		// Configure NODE_TOTAL CAN objects to receive from each Node
		for(int i = 1; i <= NODE_TOTAL; i++) {
			can_get_standard_message_filter_element_default(&res_filter[i]);
			res_filter[i].S0.bit.SFID1 = 0x100 + i;
			res_filter[i].S0.bit.SFID2 = i;
			res_filter[i].S0.bit.SFEC =
			CAN_STANDARD_MESSAGE_FILTER_ELEMENT_S0_SFEC_STRXBUF_Val;
			g_hash_res[i] = 0;
			can_set_rx_standard_filter(&can0_instance, &(res_filter[i]),i);
			can_enable_interrupt(&can0_instance, CAN_RX_BUFFER_NEW_MESSAGE);
		}
		
		//uint32_t tempVal = TIMEVAL;
		// Regenerate the server's keys for authentication
		check = InitKeys(hardcoded,secret_key,StoredPublicKeys[0],ec);
		if(!check) {
			printf("Failed server's authentication key generation!\r\n");
			return 1;
		}
	
		// Generate shared secrets using generated secret key and stored publics
		check = InitSharedSecrets(secret_key, StoredPublicKeys,NodeSharedSecrets);
		if (!check) {
			printf("Failed server's authentication shared secret generation!\r\n");
			return 1;
		}
		
		// Wait for all of the encrypted hashes to come in
		for(int i = 1; i <= NODE_TOTAL;i++) {
			//printf("Waiting for %i\r\n",i);
			while(g_hash_res[i] == 0);
		}
		
		//printf("Finished receiving responses\r\n");
		
		g_valid_nodes_mask = 0;
		// Decrypt all of the responses
		// Compare to shortened hashes to make sure they match
		for (int i = 1; i <= NODE_TOTAL; i++) {
			// Disable CAN Message
			res_filter[i].S0.bit.SFEC =
				CAN_STANDARD_MESSAGE_FILTER_ELEMENT_S0_SFEC_DISABLE_Val;
			can_set_rx_standard_filter(&can0_instance, &(res_filter[i]),i);
			// Decrypt hashed responses
			//memcpy(ReceivedResponseHashes[i],getNextBufferElement(&rx_element_buff[i])->data,16);
			printf("Decrypt: Input to message_in size: %i\r\n",mlen);
			crypto_aead_decrypt(ReceivedResponseHashes[i], &mlen, (void*)0, getNextBufferElement(&rx_element_buff[i])->data, 24, NULL, NULL, n, NodeSharedSecrets[i]);
			printf("Decrypt: Output to message_in size: %i\r\n",mlen);
			//present80Decrypt(NodeSharedSecrets[i],ReceivedResponseHashes[i]);
			//present80Decrypt(NodeSharedSecrets[i],&ReceivedResponseHashes[i][8]);
			printf("\r\NodeSharedKey = 0x");
			for(int j = 0; j < 16; j++) {
				printf("%02x",NodeSharedSecrets[i][j]);
			}
			
			printf("\r\nReceived = 0x");
			for(int j = 15; j>=0; j--) {
				printf("%02x",ReceivedResponseHashes[i][j]);
			}
			printf("\r\nStored = 0x");
			for(int j = 15; j>=0; j--) {
				printf("%02x",StoredResponseHashes[i][j]);
			}
			printf("\r\n");
			
			
			// Make sure received hash matches stored hash
			int j;
			for(j=0;j<16;j++){
				if(ReceivedResponseHashes[i][j] != StoredResponseHashes[i][j]){
					break;
				}
			}
			
			// j will be less than 16 if any part did not match
			if (j==16) {
				g_valid_nodes_mask |= (1 << i);
				//printf("Node %i is valid!\r\n",i);
			}
			else {
				printf("Node %i not valid!\r\n",i);
			}
		}
		
		// Get ready to start sending
		STAGE = SEND;
		
		// Generate Session Key (10 bytes)
		photon128((uint8_t *)&seed,1,session_key);
		memset(&session_key[10],0,6);
		//printf("\r\nGenerated session key: 0x");
		//for (int i = 9; i >= 0; i--) printf("%02x",session_key[i]);
		//printf("\r\n");
		
		// Concatenate with mask (6 bytes of space but mask is only 4 bytes)
		*((uint32_t*) &session_key[10]) |= g_valid_nodes_mask;
		
		//printf("\n\rFormatted session key and valid node concatenation complete\n\r");
		
		// Encrypt and get ready to send to each valid node
		for(int i = 1; i <=NODE_TOTAL; i++) {
			if(g_valid_nodes_mask & (1<<i)) {
				g_session_res[i] = 0;
				//memmove(EncryptedSessionKeys[i],session_key,16);
				//present80Encrypt(NodeSharedSecrets[i],EncryptedSessionKeys[i]);
				//present80Encrypt(NodeSharedSecrets[i],&EncryptedSessionKeys[i][8]);
				crypto_aead_encrypt(EncryptedSessionKeys[i], &clen, session_key, 16, NULL, NULL, NULL, n, NodeSharedSecrets[i]);
				printf("Encrypt: Node %i, Output to tx_element size: %i\r\n",i,clen);
				
				can_get_tx_buffer_element_defaults(&send_filter[i]);
				send_filter[i].T0.reg |= CAN_TX_ELEMENT_T0_STANDARD_ID(0x200 + i);
				send_filter[i].T1.reg = CAN_TX_ELEMENT_T1_FDF | CAN_TX_ELEMENT_T1_BRS |
					CAN_TX_ELEMENT_T1_DLC(CAN_TX_ELEMENT_T1_DLC_DATA16_Val);
				memcpy(send_filter[i].data,EncryptedSessionKeys[i],16);
				//can_set_tx_buffer_element(&can0_instance, &send_filter[i],
				//	i);
				//can_tx_transfer_request(&can0_instance, 1 << i);
			}
			else {
				g_session_res[i] = 2;
			}
		}
			
		//printf("Finished Encrypting session keys\r\n");
		// Send to each node
		for(int i = 1; i <= NODE_TOTAL; i++) {
			//Delay_ms(1);
			// Skip invalid nodes
			if(g_session_res[i] == 2) continue;
			
			// Send
			can_set_tx_buffer_element(&can0_instance, &send_filter[i],i);
			can_tx_transfer_request(&can0_instance, 1 << i);
			while(!(can_tx_get_transmission_status(&can0_instance) & (1 << i)));
			//Delay_ms(75);
			
			// Send second half
			/*
			memcpy(send_filter[i].data,&EncryptedSessionKeys[i][8],8);
			can_set_tx_buffer_element(&can0_instance, &send_filter[i],i);
			can_tx_transfer_request(&can0_instance, 1 << i);
			while(!(can_tx_get_transmission_status(&can0_instance) & (1 << i)));
			*/
		}
		//printf("Finished sending session keys\r\n");
		
		for(int i = 1; i <= MAX_BUFFS; i++) {
			can_get_standard_message_filter_element_default(&res_filter[i]);
			res_filter[i].S0.bit.SFID1 = 0x400+i;
			res_filter[i].S0.bit.SFID2 = i;
			res_filter[i].S0.bit.SFEC =
			CAN_STANDARD_MESSAGE_FILTER_ELEMENT_S0_SFEC_STRXBUF_Val;
			can_set_rx_standard_filter(&can0_instance, &res_filter[i],
			i);
		}
		can_enable_interrupt(&can0_instance, CAN_RX_BUFFER_NEW_MESSAGE);
		//printf("Ready to listen...\r\n");
		
		STAGE = NORMAL;
		g_normal_received_mask = 0;
		
		// Send an all clear to Nodes
		// Can just use filter object 0
		//Delay_ms(10);
		memset(message[0],0,8);
		can_get_tx_buffer_element_defaults(&tx_element);
		tx_element.T0.reg |= CAN_TX_ELEMENT_T0_STANDARD_ID(0x301);
		tx_element.T1.reg = CAN_TX_ELEMENT_T1_FDF | CAN_TX_ELEMENT_T1_BRS |
			CAN_TX_ELEMENT_T1_DLC(CAN_TX_ELEMENT_T1_DLC_DATA8_Val);
		memcpy(tx_element.data,message[0],8);
		can_set_tx_buffer_element(&can0_instance, &tx_element,
			CAN_FILTER_REGULAR_SEND);
		can_tx_transfer_request(&can0_instance, 1 << CAN_FILTER_REGULAR_SEND);
		
		while(!(can_tx_get_transmission_status(&can0_instance) & (1 << CAN_FILTER_REGULAR_SEND)));
		
		// That's it for the server's main interaction...
		//printf("That's it for Authentication. Time to listen...\r\n");
		
		uint32_t endVal = TIMEVAL;
		//printf("End: %d\r\n",endVal);
		//printf("Difference: %d\r\n",endVal-startVal);
		
		//STAGE = NORMAL;
		//g_normal_received_mask = 0;
		
// 		printf("Randomly: Node %i public key is 0x",2);
// 			for(int j = 31; j >= 0; j--) {
// 				printf("%02x ",StoredPublicKeys[2][j]);
// 			}
// 		printf("\r\n");
		
		for(int i = 0; i < TOTAL_MESSAGES; i++) {
			while(g_normal_received_mask==0);
			int flag;
			
			for(flag = 1; (g_normal_received_mask & (1<<flag))==0;flag++);
			
			//printf("%i ",flag);
			struct can_rx_element_buffer * temp = getNextBufferElement(&rx_element_buff[flag]);
			//memcpy(message[i],temp->data,8);
			//printf("MsgID=0x%08x, MSG=0x%08x.%08x\r\n",(*temp).R0.bit.ID,
			//	(*((uint32_t*)&message[flag][4])),(*(uint32_t*)message[flag]));
			g_normal_received_flags[flag]--;
			g_normal_received_mask &= ~(1<<flag);
		}

		printf("Normal operation is over\r\n");
	}
}

static void Delay_ms(uint32_t Delay) {
	printf("Delayed here");
	for(uint32_t i = 0; i < Delay*10000; i++);
}