/*
 * Enrollment_Server.c
 *
 * Created: 7/17/2021 3:43:33 AM
 *  Author: Marshmallow
 */ 
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "Photon/photon.h"
#include "FourQ/FourQ_api.h"
#include "Enrollment_Server.h"

//static volatile uint32_t g_sent = 0;
//static volatile uint32_t g_received = 0;
volatile uint32_t g_sent = 0;
volatile uint32_t g_received = 0;
volatile uint32_t flipped = 0;
uint32_t startVal;
volatile enum OPERATION STAGE = ENROLLMENT;
struct multiBuffer rx_element_buff[CONF_CAN0_RX_BUFFER_NUM];

static void Delay_ms(uint32_t ms);
uint32_t ul_tickcount = 0;
#define TIMEVAL (ul_tickcount+(SysTick->VAL)/48000)

bool EnrollNodes(int total_nodes, uint8_t (*StoredPublicKeys)[32],
	uint8_t (*StoredResponseHashes)[16], uint8_t *ec, struct can_module * can_inst){
	bool hardcoded;
	//uint8_t message[8];
	uint8_t response[16];
	//int buff_num;
	
	ECCRYPTO_STATUS Status;
	
	struct can_standard_message_filter_element sd_filter;
	struct can_tx_element tx_element;
	
	printf("\r\nWelcome to the PUF-based CAN Security Demo!\r\n");
	printf("Press 'h' to begin with hardcoded values: ");
	
	char c;
	scanf("%c", &c);
	
	while((c != 'h')) {
		printf("How could you...\r\n");
		scanf("%c", &c);
	}
	
	if(c == 'h') {
		printf("\r\nPUF responses will be hardcoded values for enrollment and authentication\r\n");
		hardcoded = true;
	}
	else {
		// Case is available if a second option becomes available
	}
	
	// Handler set to enrollment phase
	STAGE = ENROLLMENT;

	 printf("Start: %d\r\n",startVal);
	 
	// Responses will be first stored in StoredPublicKeys
	for(int i = 0; i <= total_nodes; i++) {
		memset(StoredPublicKeys[i],0,32);
	}
	
	// Let everyone know to either prepare for public keys or use hardcoded values
	// Nodes should be looking for 0x411
	can_get_tx_buffer_element_defaults(&tx_element);
	tx_element.T0.reg |= CAN_TX_ELEMENT_T0_STANDARD_ID(0x411);
	tx_element.T1.reg = CAN_TX_ELEMENT_T1_FDF | CAN_TX_ELEMENT_T1_BRS |
		CAN_TX_ELEMENT_T1_DLC(CAN_TX_ELEMENT_T1_DLC_DATA8_Val);
	memset(tx_element.data,hardcoded,8);
	can_set_tx_buffer_element(can_inst, &tx_element,
		CAN_FILTER_REGULAR_SEND);
	can_tx_transfer_request(can_inst, 1 << CAN_FILTER_REGULAR_SEND);
	
	while(!(can_tx_get_transmission_status(can_inst) & (1 << CAN_FILTER_REGULAR_SEND)));
	
	if(hardcoded) {
		//printf("Hardcoding a response\r\n");
		memset(StoredPublicKeys[0],0xFF,16);
	}
	else {
		printf("Cannot generate a response.\r\n");
	}
	
	Status = ECCRYPTO_SUCCESS;
	Status = CompressedKeyGeneration(StoredPublicKeys[0], StoredPublicKeys[0]);
	//printf("Something was supposed to happen here 1.\r\n");
	if (Status != ECCRYPTO_SUCCESS) {
		printf("Enrollment: Failed server public key generation!\r\n");
		while(1);
	}
	//printf("Milestone 1\r\n");
	//Delay_ms(1000);
	
	//can_get_tx_buffer_element_defaults(&tx_element);
	//tx_element.T0.reg |= CAN_TX_ELEMENT_T0_STANDARD_ID(0x100);
	//tx_element.T1.bit.DLC = 8;
	
	/*
	for (int i = 0; i < 3; i++) {
		memcpy(tx_element.data,&StoredPublicKeys[0][i*8],8);
		can_set_tx_buffer_element(can_inst, &tx_element,
			CAN_FILTER_REGULAR_SEND);
		can_tx_transfer_request(can_inst, 1 << CAN_FILTER_REGULAR_SEND);
		while(!(can_tx_get_transmission_status(can_inst) & (1 << CAN_FILTER_REGULAR_SEND)));
		Delay_ms(50);
	}
	*/
	memcpy(tx_element.data,&StoredPublicKeys[0][0],24);
	can_get_tx_buffer_element_defaults(&tx_element);
	tx_element.T0.reg |= CAN_TX_ELEMENT_T0_STANDARD_ID(0x100);
	tx_element.T1.reg = CAN_TX_ELEMENT_T1_FDF | CAN_TX_ELEMENT_T1_BRS |
		CAN_TX_ELEMENT_T1_DLC(CAN_TX_ELEMENT_T1_DLC_DATA24_Val);
	can_set_tx_buffer_element(can_inst,&tx_element,CAN_FILTER_REGULAR_SEND);
	can_tx_transfer_request(can_inst, 1 << CAN_FILTER_REGULAR_SEND);
	while(!(can_tx_get_transmission_status(can_inst) & (1 << CAN_FILTER_REGULAR_SEND)));
	
	// Configure to receive on CAN Object CAN_FILTER_REGULAR_REC
	can_get_standard_message_filter_element_default(&sd_filter);
	sd_filter.S0.bit.SFID1 = 0x301;
	sd_filter.S0.bit.SFID2 = CAN_FILTER_REGULAR_REC;
	sd_filter.S0.bit.SFEC =
		CAN_STANDARD_MESSAGE_FILTER_ELEMENT_S0_SFEC_STRXBUF_Val;
	//Delay_ms(20);
	for (int i = 1; i <= total_nodes; i++) {
		//printf("Now on Node %d\r\n",i);
		
		// Receive node[i]'s response on 0x200 + ID
		g_received = 0;
		can_get_standard_message_filter_element_default(&sd_filter);
		sd_filter.S0.bit.SFID1 = 0x200 + i;
		sd_filter.S0.bit.SFID2 = CAN_FILTER_REGULAR_REC;
		sd_filter.S0.bit.SFEC =
		CAN_STANDARD_MESSAGE_FILTER_ELEMENT_S0_SFEC_STRXBUF_Val;
		can_set_rx_standard_filter(can_inst, &sd_filter,
		CAN_FILTER_REGULAR_REC);
		can_enable_interrupt(can_inst, CAN_RX_BUFFER_NEW_MESSAGE);
		
		can_get_tx_buffer_element_defaults(&tx_element);
		tx_element.T0.reg |= CAN_TX_ELEMENT_T0_STANDARD_ID(0x100+i);
		tx_element.T1.reg = CAN_TX_ELEMENT_T1_FDF | CAN_TX_ELEMENT_T1_BRS |
			CAN_TX_ELEMENT_T1_DLC(CAN_TX_ELEMENT_T1_DLC_DATA8_Val);
		memcpy(tx_element.data,&StoredPublicKeys[0][24],8);
		
		can_set_tx_buffer_element(can_inst, &tx_element,
			CAN_FILTER_REGULAR_SEND);
		can_tx_transfer_request(can_inst, 1 << CAN_FILTER_REGULAR_SEND);
		
		while(!(can_tx_get_transmission_status(can_inst) & (1 << CAN_FILTER_REGULAR_SEND)));
		
		while(g_received == 0);
		g_received = 0;
		memcpy(&response[0],getNextBufferElement(&rx_element_buff[CAN_FILTER_REGULAR_REC])->data,16);
		/*
		can_set_rx_standard_filter(can_inst, &sd_filter,
			CAN_FILTER_REGULAR_REC);
		
		// Receive the rest of it
		
		printf("Half two\r\n");
		while(g_received == 1);
		g_received = 0;
		
		memcpy(&response[8],getNextBufferElement(&rx_element_buff[CAN_FILTER_REGULAR_REC])->data,8);
		printf("Received Response: ");
		for(int j = 0; j < 16; j++) {
			printf("%02x",response[j]);
		}
		printf("\r\n");
		*/
		// Lowest 16 bytes of secret will be the response
		memmove(StoredPublicKeys[i],response,16);
		
		photon128(response,16,StoredResponseHashes[i]);
		Status = CompressedKeyGeneration(StoredPublicKeys[i],StoredPublicKeys[i]);
		if (Status != ECCRYPTO_SUCCESS) {
			printf("Enrollment: Failed Node %i Public Key Generation\r\n",i);
			while(1);
		}
		else {
			//printf("Generated Node %i Key: 0x",i);
			//for (int j = 0; j < 16; j++) {
			//	printf("%02x",StoredPublicKeys[i][j]);
			//}
			//printf("\r\n");
		}
		//printf("Completed node %i enrollment\r\n",i);
	}
	
	// We're finished listening
	sd_filter.S0.bit.SFEC =
		CAN_STANDARD_MESSAGE_FILTER_ELEMENT_S0_SFEC_DISABLE_Val;
	can_set_rx_standard_filter(can_inst, &sd_filter,
		CAN_FILTER_REGULAR_REC);
		
	// Send one last 0x411 to show that enrollment has ended
	can_get_tx_buffer_element_defaults(&tx_element);
	tx_element.T0.reg |= CAN_TX_ELEMENT_T0_STANDARD_ID(0x411);
	tx_element.T1.reg = CAN_TX_ELEMENT_T1_FDF | CAN_TX_ELEMENT_T1_BRS |
		CAN_TX_ELEMENT_T1_DLC(CAN_TX_ELEMENT_T1_DLC_DATA8_Val);
	memset(tx_element.data,hardcoded,8);
	tx_element.data[0] = (uint8_t)hardcoded;
	can_set_tx_buffer_element(can_inst, &tx_element,
	CAN_FILTER_REGULAR_SEND);
	can_tx_transfer_request(can_inst, 1 << CAN_FILTER_REGULAR_SEND);
	
	while(!(can_tx_get_transmission_status(can_inst) & (1 << CAN_FILTER_REGULAR_SEND)));
	
	return hardcoded;
}

static void Delay_ms(uint32_t Delay) {
	printf("Delayed here %d",Delay);
	for(uint32_t i = 0; i < Delay*10000; i++);
}