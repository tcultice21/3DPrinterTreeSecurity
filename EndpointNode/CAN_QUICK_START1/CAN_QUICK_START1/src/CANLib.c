/*
 * CANLib.c
 *
 * Created: 4/14/2023 10:18:20 AM
 *  Author: tcultice
 */ 
#include "CANLib.h"

struct multiBuffer rx_element_buff[CONF_CAN0_RX_BUFFER_NUM];

uint8_t * CAN_Rx(uint16_t idVal, int bufferNum, struct can_module * can_inst) {
	struct can_standard_message_filter_element sd_filter;
	can_get_standard_message_filter_element_default(&sd_filter);
	sd_filter.S0.bit.SFID1 = idVal;
	sd_filter.S0.bit.SFID2 = bufferNum;
	sd_filter.S0.bit.SFEC = CAN_STANDARD_MESSAGE_FILTER_ELEMENT_S0_SFEC_STRXBUF_Val;
	can_set_rx_standard_filter(can_inst, &sd_filter,
		bufferNum);
	can_enable_interrupt(can_inst, CAN_RX_BUFFER_NEW_MESSAGE);
	return rx_element_buff[bufferNum].buffers[rx_element_buff[bufferNum].last_read].data;
}

uint8_t *CAN_Rx_FIFO(uint16_t idVal, uint16_t maskVal, uint16_t filterVal, struct can_module * can_inst) {
	struct can_standard_message_filter_element sd_filter;
	
	can_get_standard_message_filter_element_default(&sd_filter);
	sd_filter.S0.bit.SFID1 = idVal;
	sd_filter.S0.bit.SFID2 = maskVal;
	can_set_rx_standard_filter(can_inst, &sd_filter,
		filterVal);
	can_enable_interrupt(can_inst, CAN_RX_FIFO_0_NEW_MESSAGE);
	return rx_element_buff[filterVal].buffers[rx_element_buff[filterVal].last_read].data;
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
	return 0;
}

int CAN_Tx(uint16_t idVal, uint8_t *data, uint32_t dataLen, int buffer, struct can_module * can_inst) {
	struct can_tx_element tx_element;
	if (dataLen > 56 || dataLen < 8 || dataLen % 8 != 0) {
		printf("WARNING: CAN_Tx received value of %d",dataLen);
		return -1;
	}
	
	memset(tx_element.data,0,64);
	memcpy(tx_element.data,data,dataLen);
	CAN_Tx_Raw(idVal,&tx_element,dataLen,buffer,can_inst);
	
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
	return 1;
}