/*
 * Enrollment.h
 *
 * Created: 7/10/2021 12:20:11 AM
 *  Author: Marshmallow
 */ 


#ifndef ENROLLMENT_H_
#define ENROLLMENT_H_

#include <stdlib.h>
#include <stdint.h>
#include <conf_can.h>
#include <string.h>
#include <asf.h>
#include <stdbool.h>
#include "ASCON/api.h"
#include "ASCON/ascon.h"
#include "ASCON/crypto_aead.h"

#define CAN_FILTER_ENROLLMENT 3
#define CAN_FILTER_PUBLICKEY 4
#define CAN_TX_FILTER_BUFFER_INDEX 5
#define MAX_BUFFS 4

enum OPERATION {
	ENROLLMENT,
	AUTHENTICATION,
	NORMAL
};

struct multiBuffer {
	uint8_t last_write;
	uint8_t last_read;
	struct can_rx_element_buffer buffers[MAX_BUFFS];
} typedef multiBuffer;

extern volatile enum OPERATION STAGE;
extern struct multiBuffer rx_element_buff[CONF_CAN0_RX_BUFFER_NUM];
extern volatile unsigned char n[CRYPTO_NPUBBYTES];
extern volatile uint32_t g_rec;
extern volatile uint32_t g_sent;
extern volatile uint32_t g_rec_public;
extern uint32_t startVal;
extern volatile uint32_t ul_tickcount;
uint8_t Enrollment(uint8_t node_id, uint8_t *secret_key, uint8_t *ServerPublicKey, uint8_t *ec, struct can_module * can_inst);
// TODO: Probably use as "counter value"

inline struct can_rx_element_buffer * getNextBufferElement(struct multiBuffer * buff) {
	if (buff->last_write == buff->last_read) return NULL;
	register int last_element = buff->last_read;
	
	buff->last_read = (buff->last_read + 1) % MAX_BUFFS;
	
	return &(buff->buffers[last_element]);
}

#endif /* ENROLLMENT_H_ */