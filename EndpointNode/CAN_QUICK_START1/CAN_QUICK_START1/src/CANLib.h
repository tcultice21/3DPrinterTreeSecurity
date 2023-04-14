/*
 * CANLib.h
 *
 * Created: 4/14/2023 10:18:02 AM
 *  Author: tcultice
 */ 


#ifndef CANLIB_H_
#define CANLIB_H_

typedef address uint16_t;

// Probably not mine?
typedef struct location {
	address outAddr;
	multiBuffer * recBuff;
	
	int inCounter;
	int outCounter;
	
	
};

// TODO: Make a union that provides the metadata needed to properly sort/identify the packet style, etc.


struct multiBuffer {
	uint8_t last_write;
	uint8_t last_read;
	struct can_rx_element_buffer buffers[MAX_BUFFS];
} typedef multiBuffer;

extern struct multiBuffer rx_element_buff[CONF_CAN0_RX_BUFFER_NUM];

uint8_t CAN_Rx_Disable(int bufferNum, struct can_module * can_inst);
uint8_t * CAN_Rx(uint16_t idVal, int bufferNum, struct can_module * can_inst);

inline void CAN_Tx_Wait(int buffer, struct can_module * can_inst) {
	while(!(can_tx_get_transmission_status(can_inst) & (1 << buffer)));
}

inline struct can_rx_element_buffer * getNextBufferElement(struct multiBuffer * buff) {
	if (buff->last_write == buff->last_read) return NULL;
	register int last_element = buff->last_read;
	
	buff->last_read = (buff->last_read + 1) % MAX_BUFFS;
	
	return &(buff->buffers[last_element]);
}

#endif /* CANLIB_H_ */