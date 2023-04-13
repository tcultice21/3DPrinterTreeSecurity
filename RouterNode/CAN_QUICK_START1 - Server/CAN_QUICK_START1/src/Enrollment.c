/*
 * Enrollment.c
 *
 * Created: 7/10/2021 12:19:58 AM
 *  Author: Marshmallow
 */ 

#include "Enrollment.h"

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
	can_get_standard_message_filter_element_default(&sd_filter);
	sd_filter.S0.bit.SFID2 = CAN_FILTER_ENROLLMENT;
	sd_filter.S0.bit.SFID1 = 0x411;
	sd_filter.S0.bit.SFEC =
		CAN_STANDARD_MESSAGE_FILTER_ELEMENT_S0_SFEC_STRXBUF_Val;
	can_set_rx_standard_filter(can_inst, &sd_filter,
		CAN_FILTER_ENROLLMENT);
	can_enable_interrupt(can_inst, CAN_RX_BUFFER_NEW_MESSAGE);

	while(g_rec == 0) {}
	g_rec = 0;
	
	can_get_standard_message_filter_element_default(&sd_filter);
	sd_filter.S0.bit.SFEC =
		CAN_STANDARD_MESSAGE_FILTER_ELEMENT_S0_SFEC_DISABLE_Val;
	can_set_rx_standard_filter(can_inst, &sd_filter,
		CAN_FILTER_ENROLLMENT);
		
	if(hardcoded) {
		printf("Using hardcoded values...\r\n");
		
		memset(secret_key+16,0,16);
		memset(secret_key,node_id,16);
	}
	else {
		printf("PUF stuff not available.\r\n");
		memset(secret_key,0,32);
	}
	
	// Get Public Key (3/4 parts of it)
	can_get_standard_message_filter_element_default(&sd_filter);
	sd_filter.S0.bit.SFID2 = CAN_FILTER_PUBLICKEY;
	sd_filter.S0.bit.SFID1 = 0x100;
	sd_filter.S0.bit.SFEC =
		CAN_STANDARD_MESSAGE_FILTER_ELEMENT_S0_SFEC_STRXBUF_Val;
	can_set_rx_standard_filter(can_inst, &sd_filter,
		CAN_FILTER_PUBLICKEY);
	can_enable_interrupt(can_inst, CAN_RX_BUFFER_NEW_MESSAGE);
	
	int i;
	for(i=0; i<3; i++) {
		while(g_rec_public == (unsigned)i);
		memcpy(&ServerPublicKey[i*8],rx_element_buff[CAN_FILTER_PUBLICKEY].data,8);
	}
	
	// Get last, special part of the server public key (4/4)
	can_get_standard_message_filter_element_default(&sd_filter);
	sd_filter.S0.bit.SFID1 = 0x100 + node_id;
	sd_filter.S0.bit.SFID2 = CAN_FILTER_PUBLICKEY;
	sd_filter.S0.bit.SFEC =
		CAN_STANDARD_MESSAGE_FILTER_ELEMENT_S0_SFEC_STRXBUF_Val;
	can_set_rx_standard_filter(can_inst, &sd_filter,
		CAN_FILTER_PUBLICKEY);
	can_enable_interrupt(can_inst, CAN_RX_BUFFER_NEW_MESSAGE);

	while(g_rec_public == i);
	memcpy(&ServerPublicKey[i*8],rx_element_buff[CAN_FILTER_PUBLICKEY].data,8);
	
	can_get_standard_message_filter_element_default(&sd_filter);
	sd_filter.S0.bit.SFEC =
		CAN_STANDARD_MESSAGE_FILTER_ELEMENT_S0_SFEC_DISABLE_Val;
	can_set_rx_standard_filter(can_inst, &sd_filter,
		CAN_FILTER_PUBLICKEY);
	
	printf("Ready to transmit response\r\n");
	
	// Send the response (2 packets)
	struct can_tx_element tx_element;

	// Part 1:
	can_get_tx_buffer_element_defaults(&tx_element);
	tx_element.T0.reg |= CAN_TX_ELEMENT_T0_STANDARD_ID(0x200+node_id);
	tx_element.T1.bit.DLC = 8;
	memcpy(tx_element.data,secret_key,8);
	can_set_tx_buffer_element(can_inst, &tx_element,
		CAN_TX_FILTER_BUFFER_INDEX);
	can_tx_transfer_request(can_inst, 1 << CAN_TX_FILTER_BUFFER_INDEX);
	
	while(can_tx_get_transmission_status(can_inst) & (1 << CAN_TX_FILTER_BUFFER_INDEX));
	
	// best delay function 2021
	for(i = 0; i < 300000; i++);
	
	// Part 2:
	can_get_tx_buffer_element_defaults(&tx_element);
	tx_element.T0.reg |= CAN_TX_ELEMENT_T0_STANDARD_ID(0x200+node_id);
	tx_element.T1.bit.DLC = 8;
	memcpy(tx_element.data,&secret_key[8],8);
	can_set_tx_buffer_element(can_inst, &tx_element,
		CAN_TX_FILTER_BUFFER_INDEX);
	can_tx_transfer_request(can_inst, 1 << CAN_TX_FILTER_BUFFER_INDEX);
	
	while(can_tx_get_transmission_status(can_inst) & (1 << CAN_TX_FILTER_BUFFER_INDEX));
	
	
	// Wait for final message from server confirming enrollment has completed
	printf("Last part\r\n");
	can_get_standard_message_filter_element_default(&sd_filter);
	sd_filter.S0.bit.SFID2 = CAN_FILTER_ENROLLMENT;
	sd_filter.S0.bit.SFID1 = 0x411;
	sd_filter.S0.bit.SFEC =
		CAN_STANDARD_MESSAGE_FILTER_ELEMENT_S0_SFEC_STRXBUF_Val;
	can_set_rx_standard_filter(can_inst, &sd_filter,
		CAN_FILTER_ENROLLMENT);
	can_enable_interrupt(can_inst, CAN_RX_BUFFER_NEW_MESSAGE);
	
	while(g_rec == 0);
	g_rec = 0;
	
	can_get_standard_message_filter_element_default(&sd_filter);
	sd_filter.S0.bit.SFEC =
		CAN_STANDARD_MESSAGE_FILTER_ELEMENT_S0_SFEC_DISABLE_Val;
	can_set_rx_standard_filter(can_inst, &sd_filter,
		CAN_FILTER_ENROLLMENT);

	return hardcoded;
}