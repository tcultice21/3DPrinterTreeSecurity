/*
 * CANLib.c
 *
 * Created: 4/23/2023 12:21:53 AM
 *  Author: Marshmallow
 */ 
#include "CANLib.h"

struct can_module can0_instance;
struct multiBuffer CAN0_rx_element_buff[NETWORK_MAX_BUFFS];

#ifdef SYSTEM_ROUTER_TYPE
struct can_module can1_instance;
struct multiBuffer CAN1_rx_element_buff[NETWORK_MAX_BUFFS];
#endif

//struct multiBuffer rx_element_buff[CONF_CAN0_RX_BUFFER_NUM];

uint8_t * CAN_Rx(uint16_t idVal, int bufferNum, struct can_module * can_inst) {
	struct can_standard_message_filter_element sd_filter;
	can_get_standard_message_filter_element_default(&sd_filter);
	sd_filter.S0.bit.SFID1 = idVal;
	sd_filter.S0.bit.SFID2 = bufferNum;
	sd_filter.S0.bit.SFEC = CAN_STANDARD_MESSAGE_FILTER_ELEMENT_S0_SFEC_STRXBUF_Val;
	can_set_rx_standard_filter(can_inst, &sd_filter,
		bufferNum);
	can_enable_interrupt(can_inst, CAN_RX_BUFFER_NEW_MESSAGE);
	return NULL;//rx_element_buff[bufferNum].buffers[rx_element_buff[bufferNum].last_read].data;
}

uint8_t *CAN_Rx_FIFO(uint16_t idVal, uint16_t maskVal, uint16_t filterVal, struct can_module * can_inst) {
	struct can_standard_message_filter_element sd_filter;
	
	can_get_standard_message_filter_element_default(&sd_filter);
	sd_filter.S0.bit.SFID1 = idVal;
	sd_filter.S0.bit.SFID2 = maskVal;
	can_set_rx_standard_filter(can_inst, &sd_filter,
		filterVal);
	can_enable_interrupt(can_inst, CAN_RX_FIFO_0_NEW_MESSAGE);
	return NULL;//rx_element_buff[filterVal].buffers[rx_element_buff[filterVal].last_read].data;
}

uint8_t CAN_Rx_Disable(int bufferNum, struct can_module * can_inst) {
	struct can_standard_message_filter_element sd_filter;
	can_get_standard_message_filter_element_default(&sd_filter);
	sd_filter.S0.bit.SFEC =
		CAN_STANDARD_MESSAGE_FILTER_ELEMENT_S0_SFEC_DISABLE_Val;
	can_set_rx_standard_filter(can_inst, &sd_filter,
		bufferNum);
	return 0;
}

int num_to_CAN(int num) {
	if (num <= 8) {
		return 12;
	}
	else if (num <= 24) {
		return ((num+7)>>2)+5;
	}
	else {
		return ((num+15)>>4)+11;
	}
}

const char map[16] = {-1,1,2,3,4,5,6,7,8,12,16,20,24,32,48,64};

int CAN_Tx_Raw(uint16_t idVal, struct can_tx_element * tx_element, uint32_t dataLen, int buffer, struct can_module * can_inst) {
	can_get_tx_buffer_element_defaults(tx_element);
	tx_element->T0.reg = CAN_TX_ELEMENT_T0_STANDARD_ID(idVal);
	tx_element->T1.reg = CAN_TX_ELEMENT_T1_FDF | CAN_TX_ELEMENT_T1_BRS;
	switch(map[num_to_CAN(dataLen)]) {
		case 12:
			tx_element->T1.reg |= CAN_TX_ELEMENT_T1_DLC(CAN_TX_ELEMENT_T1_DLC_DATA12_Val);
			break;
		case 16:
			tx_element->T1.reg |= CAN_TX_ELEMENT_T1_DLC(CAN_TX_ELEMENT_T1_DLC_DATA16_Val);
			break;
		case 20:
			tx_element->T1.reg |= CAN_TX_ELEMENT_T1_DLC(CAN_TX_ELEMENT_T1_DLC_DATA20_Val);
			break;
		case 24:
			tx_element->T1.reg |= CAN_TX_ELEMENT_T1_DLC(CAN_TX_ELEMENT_T1_DLC_DATA24_Val);
			break;
		case 32:
			tx_element->T1.reg |= CAN_TX_ELEMENT_T1_DLC(CAN_TX_ELEMENT_T1_DLC_DATA32_Val);
			break;
		case 48:
			tx_element->T1.reg |= CAN_TX_ELEMENT_T1_DLC(CAN_TX_ELEMENT_T1_DLC_DATA48_Val);
			break;
		case 64:
			tx_element->T1.reg |= CAN_TX_ELEMENT_T1_DLC(CAN_TX_ELEMENT_T1_DLC_DATA64_Val);
			break;
		default:
			printf("An unexpected value was encountered in CAN_Tx_Raw, %d %d.\r\n",dataLen,map[num_to_CAN(dataLen)]);
			return 3;
	}
	can_set_tx_buffer_element(can_inst, tx_element,
		buffer);
	can_tx_transfer_request(can_inst, 1 << buffer);
	//can_enable_interrupt(can_inst, CAN_TX_EVENT_FIFO_NEW_ENTRY);
	return map[num_to_CAN(dataLen)];
}

int CAN_Tx(uint16_t idVal, uint8_t *data, uint32_t dataLen, int buffer, struct can_module * can_inst) {
	struct can_tx_element tx_element;
	/*if (dataLen > 56 || dataLen < 8 || dataLen % 8 != 0) {
		printf("WARNING: CAN_Tx received value of %d",dataLen);
	}*/
	
	memset(tx_element.data,0,64);
	memcpy(tx_element.data,data,dataLen);
	return CAN_Tx_Raw(idVal,&tx_element,dataLen,buffer,can_inst);
	
	/*can_get_tx_buffer_element_defaults(&tx_element);
	tx_element.T0.reg |= CAN_TX_ELEMENT_T0_STANDARD_ID(idVal);
	tx_element.T1.reg = CAN_TX_ELEMENT_T1_FDF | CAN_TX_ELEMENT_T1_BRS |
	CAN_TX_ELEMENT_T1_DLC(num_to_CAN(dataLen));
	memset(&(tx_element.data[dataLen]),0,map[num_to_CAN(dataLen)]-dataLen);
	memcpy(tx_element.data,data,dataLen);
	//printf("Transmitting: ");
	//for (int i = 0; i < len; i++) printf("%02x",tx_element.data[i]);
	//printf("\r\n");
	can_set_tx_buffer_element(can_inst, &tx_element,
	buffer);
	can_tx_transfer_request(can_inst, 1 << buffer);
	while(!(can_tx_get_transmission_status(can_inst) & (1 << buffer)));
	can_enable_interrupt(can_inst, CAN_TX_EVENT_FIFO_NEW_ENTRY);*/
}

int network_start_listening(struct network* network, struct network_addr* selfAddr) {
	// This is trying to setup the broadcast listen.
	if(selfAddr->CAN_addr == BROADCAST_ID) {
		CAN_Rx(selfAddr->CAN_addr,network->broadcast_buff_num, network->can_instance);
		return 1;
	}
	// This is trying to setup a different listen.
	CAN_Rx(selfAddr->CAN_addr,network->buff_num, network->can_instance);
	return 0;
}

// Tx only uses one Buff per network
// Rx receives on multiple buffers based on broadcast or not (is set up on its own)
void network_send(struct network* network, struct network_addr* addr, uint8_t* data, size_t len) {
	struct can_module * can_instance = network->can_instance;
	debug_print("Network send - Message: \n");
	for (int sex = 0; sex < len; sex++) {
		printf("%x ",data[sex]);
	}
	printf("\r\n");
	CAN_Tx(addr->CAN_addr,data,len,network->buff_num,can_instance);
	CAN_Tx_Wait(network->buff_num,can_instance);
	debug_print("Successfully Sent Data to ID %d.\n", addr->CAN_addr);
}

int network_check_any(struct network* network, struct network_addr* source, uint8_t* buff, size_t len) {
	(void)source;
	//struct node_msg_generic structBuf;
	unsigned long mlen;
	
	
	if (network->rx_element_buff[network->buff_num].last_read != network->rx_element_buff[network->buff_num].last_write) {
		debug_print("Received regular data.\n");
		struct can_rx_element_buffer * message = getNextBufferElement(&(network->rx_element_buff[network->buff_num]));
		uint8_t dataLen = ((DLC_to_Val(message->R1.bit.DLC) < len) ? DLC_to_Val(message->R1.bit.DLC) : len);
		
		printf("DEBUG: DLC to Val: %d\r\n",DLC_to_Val(message->R1.bit.DLC));
		memcpy(buff,message->data,dataLen);
		debug_print("Network Receive - Message: \n");
		for (int sex = 0; sex < dataLen; sex++) {
			printf("%x ",buff[sex]);
		}
		printf("\r\n");
		return dataLen;
	}
	else if (network->broadcast_buff_num != (uint16_t)-1U && network->rx_element_buff[network->broadcast_buff_num].last_read != network->rx_element_buff[network->broadcast_buff_num].last_write) {
		debug_print("Received broadcast data.\n");
		struct can_rx_element_buffer * message = getNextBufferElement(&(network->rx_element_buff[network->broadcast_buff_num]));
		uint8_t dataLen = ((DLC_to_Val(message->R1.bit.DLC) < len) ? DLC_to_Val(message->R1.bit.DLC) : len);
		
		printf("DEBUG: DLC to Val: %d\r\n",DLC_to_Val(message->R1.bit.DLC));
		memcpy(buff,message->data,dataLen);
		debug_print("Broadcast Network Receive - Message: \n");
		for (int sex = 0; sex < dataLen; sex++) {
			printf("%x ",buff[sex]);
		}
		printf("\r\n");
		return dataLen;
	}
	else {
		return 0;
	}
}