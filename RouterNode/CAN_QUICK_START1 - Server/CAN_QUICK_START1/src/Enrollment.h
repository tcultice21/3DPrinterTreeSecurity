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

#define CAN_FILTER_ENROLLMENT 3
#define CAN_FILTER_PUBLICKEY 4
#define CAN_TX_FILTER_BUFFER_INDEX 5

enum OPERATION {
	ENROLLMENT,
	AUTHENTICATION,
	NORMAL
};

static volatile enum OPERATION STAGE = ENROLLMENT;
static struct can_rx_element_buffer rx_element_buff[CONF_CAN0_RX_BUFFER_NUM];
static volatile uint32_t g_rec = 0;
static volatile uint32_t g_sent = 0;
static volatile uint32_t g_rec_public = 0;

uint8_t Enrollment(uint8_t node_id, uint8_t *secret_key, uint8_t *ServerPublicKey, uint8_t *ec, struct can_module * can_inst);



#endif /* ENROLLMENT_H_ */