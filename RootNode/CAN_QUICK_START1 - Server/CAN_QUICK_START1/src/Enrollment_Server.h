/*
 * Enrollment_Server.h
 *
 * Created: 7/17/2021 3:43:53 AM
 *  Author: Marshmallow
 */ 

#ifndef ENROLLMENT_SERVER_H_
#define ENROLLMENT_SERVER_H_

#include "EncLib.h"
#include "CANLib.h"

enum OPERATION {
	ENROLLMENT,
	RECEIVE,
	SEND,
	NORMAL
};

#include <stdlib.h>
#include <stdint.h>
#include <conf_can.h>
#include <string.h>
#include <asf.h>
#include <stdbool.h>

#define CAN_FILTER_REGULAR_SEND 1
#define CAN_FILTER_REGULAR_REC 2
#define MAX_BUFFS 4

extern uint32_t startVal;
extern uint32_t ul_tickcount;

/*struct multiBuffer {
	uint8_t last_write;
	uint8_t last_read;
	struct can_rx_element_buffer buffers[MAX_BUFFS];
} typedef multiBuffer;*/

extern volatile uint32_t g_sent;
extern volatile uint32_t g_received;
extern volatile uint32_t flipped;
volatile enum OPERATION STAGE;
//extern struct multiBuffer rx_element_buff[CONF_CAN0_RX_BUFFER_NUM];

bool EnrollNodes(int total_nodes, uint8_t (*StoredPublicKeys)[32],
	uint8_t (*StoredResponseHashes)[16], uint8_t *ec, struct can_module * can_inst);



/*inline struct can_rx_element_buffer * getNextBufferElement(struct multiBuffer * buff) {
	if (buff->last_write == buff->last_read) return NULL;
	register int last_element = buff->last_read;
	
	buff->last_read = (buff->last_read + 1) % MAX_BUFFS;
	
	return &(buff->buffers[last_element]);
}*/

#endif /* ENROLLMENT_SERVER_H_ */