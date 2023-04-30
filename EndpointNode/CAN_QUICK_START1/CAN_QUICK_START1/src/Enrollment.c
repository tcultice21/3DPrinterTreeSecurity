/*
 * Enrollment.c
 *
 * Created: 7/10/2021 12:19:58 AM
 *  Author: Marshmallow
 */ 

#include "Enrollment.h"
#include "EncLib.h"

uint32_t startVal;
volatile uint32_t ul_tickcount = 0;

#define TIMEVAL (ul_tickcount+(14400000UL-1UL-SysTick->VAL)/48000UL)


volatile uint32_t g_rec = 0;
volatile uint32_t g_sent = 0;
volatile uint32_t g_rec_public = 0;
//volatile unsigned char n[CRYPTO_NPUBBYTES] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};

// Important information about node's self
//struct selfInfo selfData = {.nonce = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15}};


volatile enum OPERATION STAGE = ENROLLMENT;
//struct multiBuffer rx_element_buff[CONF_CAN0_RX_BUFFER_NUM];

uint8_t Enrollment(uint8_t node_id, uint8_t *secret_key, uint8_t *ServerPublicKey, uint8_t *ec, struct can_module * can_inst) {
	// Send_Obj - FIFO 0
	// Recv_Obj - Buffer 4
	// Using filter 3
	bool hardcoded = true;
	uint8_t message[8];
	
	memset(message,0,8);
	STAGE = ENROLLMENT;
	
	struct can_standard_message_filter_element sd_filter;
	
	// Get Enrollment verification
	CAN_Rx(0x411,CAN_FILTER_ENROLLMENT,can_inst);
	/*can_get_standard_message_filter_element_default(&sd_filter);
	sd_filter.S0.bit.SFID2 = CAN_FILTER_ENROLLMENT;
	sd_filter.S0.bit.SFID1 = 0x411;
	sd_filter.S0.bit.SFEC =
		CAN_STANDARD_MESSAGE_FILTER_ELEMENT_S0_SFEC_STRXBUF_Val;
	can_set_rx_standard_filter(can_inst, &sd_filter,
		CAN_FILTER_ENROLLMENT);
	can_enable_interrupt(can_inst, CAN_RX_BUFFER_NEW_MESSAGE);*/

	// Get Public Key (3/4 parts of it)
	CAN_Rx(0x100,CAN_FILTER_PUBLICKEY,can_inst);
	/*can_get_standard_message_filter_element_default(&sd_filter);
	sd_filter.S0.bit.SFID2 = CAN_FILTER_PUBLICKEY;
	sd_filter.S0.bit.SFID1 = 0x100;
	sd_filter.S0.bit.SFEC =
	CAN_STANDARD_MESSAGE_FILTER_ELEMENT_S0_SFEC_STRXBUF_Val;
	can_set_rx_standard_filter(can_inst, &sd_filter,
	CAN_FILTER_PUBLICKEY);
	can_enable_interrupt(can_inst, CAN_RX_BUFFER_NEW_MESSAGE);*/
	
	while(g_rec == 0);
	g_rec = 0;
	hardcoded = (bool)(getNextBufferElement(&rx_element_buff[CAN_FILTER_ENROLLMENT])->data[0]);
	
// 	can_get_standard_message_filter_element_default(&sd_filter);
// 	sd_filter.S0.bit.SFEC =
// 		CAN_STANDARD_MESSAGE_FILTER_ELEMENT_S0_SFEC_DISABLE_Val;
// 	can_set_rx_standard_filter(can_inst, &sd_filter,
// 		CAN_FILTER_ENROLLMENT);
	
	startVal = TIMEVAL;
	printf("Start: %d\r\n",startVal);
	
	if(hardcoded) {
		printf("Using hardcoded values...\r\n");
		
		memset(secret_key+16,0,16);
		memset(secret_key,node_id,16);
	}
	else {
		printf("PUF stuff not available.\r\n");
		memset(secret_key,0,32);
	}
	
	// Get last, special part of the server public key (4/4)
	
	CAN_Rx(0x100 + node_id,CAN_FILTER_PUBLICKEY+1,can_inst);
	//can_get_standard_message_filter_element_default(&sd_filter);
	//sd_filter.S0.bit.SFID1 = 0x100 + node_id;
	//sd_filter.S0.bit.SFID2 = CAN_FILTER_PUBLICKEY;
	//sd_filter.S0.bit.SFEC =
	//CAN_STANDARD_MESSAGE_FILTER_ELEMENT_S0_SFEC_STRXBUF_Val;
	//can_set_rx_standard_filter(can_inst, &sd_filter,
	//CAN_FILTER_PUBLICKEY+1);
	//can_enable_interrupt(can_inst, CAN_RX_BUFFER_NEW_MESSAGE);
	//int i;
	// CAN FD change - Receive all 3 parts simultaneously.
	/*
	for(i=0; i<3; i++) {
		while(g_rec_public == (unsigned)i);
		memcpy(&ServerPublicKey[i*8],getNextBufferElement(&rx_element_buff[CAN_FILTER_PUBLICKEY])->data,8);
	}
	*/
	while(g_rec_public <= 1);
	
	//printf("Made it here.\r\n");

	//while(g_rec_public == 1);
	memcpy(&ServerPublicKey[0],getNextBufferElement(&rx_element_buff[CAN_FILTER_PUBLICKEY])->data,24);
	memcpy(&ServerPublicKey[24],getNextBufferElement(&rx_element_buff[CAN_FILTER_PUBLICKEY+1])->data,8);
	
	//printf("Made it here 2.\r\n");
	
// 	can_get_standard_message_filter_element_default(&sd_filter);
// 	sd_filter.S0.bit.SFEC =
// 		CAN_STANDARD_MESSAGE_FILTER_ELEMENT_S0_SFEC_DISABLE_Val;
// 	can_set_rx_standard_filter(can_inst, &sd_filter,
// 		CAN_FILTER_PUBLICKEY);
	
	//printf("Ready to transmit response\r\n");
	//printf("Response: ");
	//for(i = 0; i < 16; i++) {
	//	printf("%02x",secret_key[i]);
	//}
	//printf("\r\n");
	
	// Send the response (2 packets)
	struct can_tx_element tx_element;

	// Part 1:
	CAN_Tx(0x200+node_id,secret_key,16,CAN_TX_FILTER_BUFFER_INDEX,can_inst);
	/*can_get_tx_buffer_element_defaults(&tx_element);
	tx_element.T0.reg |= CAN_TX_ELEMENT_T0_STANDARD_ID(0x200+node_id);
	tx_element.T1.reg = CAN_TX_ELEMENT_T1_FDF | CAN_TX_ELEMENT_T1_BRS |
		CAN_TX_ELEMENT_T1_DLC(CAN_TX_ELEMENT_T1_DLC_DATA16_Val);
	memcpy(tx_element.data,secret_key,16);
	can_set_tx_buffer_element(can_inst, &tx_element,
		CAN_TX_FILTER_BUFFER_INDEX);
	can_tx_transfer_request(can_inst, 1 << CAN_TX_FILTER_BUFFER_INDEX);*/
	printf("Waiting for transfer.");
	CAN_Tx_Wait(CAN_TX_FILTER_BUFFER_INDEX,can_inst);
	while(!(can_tx_get_transmission_status(can_inst) & (1 << CAN_TX_FILTER_BUFFER_INDEX)));
	
	printf("Transfer Complete.\r\n");
	// best delay function 2021
	//for(i = 0; i < 300000; i++);
	
	/*
	// Part 2:
	can_get_tx_buffer_element_defaults(&tx_element);
	tx_element.T0.reg |= CAN_TX_ELEMENT_T0_STANDARD_ID(0x200+node_id);
	tx_element.T1.bit.DLC = 8;
	memcpy(tx_element.data,&secret_key[8],8);
	can_set_tx_buffer_element(can_inst, &tx_element,
		CAN_TX_FILTER_BUFFER_INDEX);
	can_tx_transfer_request(can_inst, 1 << CAN_TX_FILTER_BUFFER_INDEX);
	
	while(!(can_tx_get_transmission_status(can_inst) & (1 << CAN_TX_FILTER_BUFFER_INDEX)));
	*/
	
	// Wait for final message from server confirming enrollment has completed
	//printf("Last part\r\n");
	CAN_Rx(0x411,CAN_FILTER_ENROLLMENT,can_inst);
	/*can_get_standard_message_filter_element_default(&sd_filter);
	sd_filter.S0.bit.SFID2 = CAN_FILTER_ENROLLMENT;
	sd_filter.S0.bit.SFID1 = 0x411;
	sd_filter.S0.bit.SFEC =
		CAN_STANDARD_MESSAGE_FILTER_ELEMENT_S0_SFEC_STRXBUF_Val;
	can_set_rx_standard_filter(can_inst, &sd_filter,
		CAN_FILTER_ENROLLMENT);
	can_enable_interrupt(can_inst, CAN_RX_BUFFER_NEW_MESSAGE);*/
	
	while(g_rec == 0);
	g_rec = 0;
	CAN_Rx_Disable(CAN_FILTER_ENROLLMENT,can_inst);
	CAN_Rx_Disable(CAN_FILTER_PUBLICKEY,can_inst);
	CAN_Rx_Disable(CAN_FILTER_PUBLICKEY+1,can_inst);
	/*can_get_standard_message_filter_element_default(&sd_filter);
	sd_filter.S0.bit.SFEC =
		CAN_STANDARD_MESSAGE_FILTER_ELEMENT_S0_SFEC_DISABLE_Val;
	can_set_rx_standard_filter(can_inst, &sd_filter,
		CAN_FILTER_ENROLLMENT);*/

	return hardcoded;
}