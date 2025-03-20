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
#include "ASCON/constants.h"
#include "CANLib.h"

#define CAN_FILTER_ENROLLMENT 3
#define CAN_FILTER_PUBLICKEY 4
#define CAN_TX_FILTER_BUFFER_INDEX 5
#define MAX_BUFFS 4

enum OPERATION {
	ENROLLMENT,
	AUTHENTICATION,
	NORMAL
};

/*typedef struct selfInfo {
	int idIn; // Assigned ID value to input receive on
	uint8_t secret_key[32];
	uint8_t public_key[32];
	uint8_t shared_secret[32];
	uint8_t shared_hash[16];
	uint8_t session_key[ASCON_128_KEYBYTES];
	volatile uint8_t nonce[CRYPTO_NPUBBYTES];
} selfInfo;*/

//extern struct selfInfo selfData;

// Authentication data/counters
extern volatile enum OPERATION STAGE;
extern volatile uint32_t g_rec;
extern volatile uint32_t g_sent;
extern volatile uint32_t g_rec_public;
extern uint32_t startVal;
extern volatile uint32_t ul_tickcount;


uint8_t Enrollment(uint8_t node_id, uint8_t *secret_key, uint8_t *ServerPublicKey, uint8_t *ec, struct can_module * can_inst);

#endif /* ENROLLMENT_H_ */