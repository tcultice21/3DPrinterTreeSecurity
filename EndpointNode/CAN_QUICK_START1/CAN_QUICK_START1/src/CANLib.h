/*
 * CANLib.h
 *
 * Created: 4/14/2023 10:18:02 AM
 *  Author: tcultice
 */ 


#ifndef CANLIB_H_
#define CANLIB_H_

#include <stdlib.h>
#include <stdint.h>
#include <conf_can.h>
#include <string.h>
#include <asf.h>
#include <stdbool.h>
#define MAX_BUFFS 4

struct multiBuffer {
	uint8_t last_write;
	uint8_t last_read;
	struct can_rx_element_buffer buffers[MAX_BUFFS];
} typedef multiBuffer;

extern struct multiBuffer rx_element_buff[CONF_CAN0_RX_BUFFER_NUM];

// Rx Functions
uint8_t CAN_Rx_Disable(int bufferNum, struct can_module * can_inst);
uint8_t * CAN_Rx(uint16_t idVal, int bufferNum, struct can_module * can_inst);
uint8_t *CAN_Rx_FIFO(uint16_t idVal, uint16_t maskVal, uint16_t filterVal, struct can_module * can_inst);

// Tx Functions
int CAN_Tx_Raw(uint16_t idVal, struct can_tx_element * tx_element, uint32_t dataLen, int buffer, struct can_module * can_inst);
//int CAN_Tx_Raw(uint16_t idVal, struct can_tx_element tx_element, uint32_t dataLen, int buffer, struct can_module * can_inst);
int CAN_Tx(uint16_t idVal, uint8_t *data, uint32_t dataLen, int buffer, struct can_module * can_inst);

inline static void CAN_Tx_Wait(int buffer, struct can_module * can_inst) {
	while(!(can_tx_get_transmission_status(can_inst) & (1 << buffer)));
}

inline struct can_rx_element_buffer * getNextBufferElement(struct multiBuffer * buff) {
	if (buff->last_write == buff->last_read) return NULL;
	register int last_element = buff->last_read;
	
	buff->last_read = (buff->last_read + 1) % MAX_BUFFS;
	
	return &(buff->buffers[last_element]);
}

#endif /* CANLIB_H_ */