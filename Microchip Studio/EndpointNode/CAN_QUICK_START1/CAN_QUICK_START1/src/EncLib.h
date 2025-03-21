/*
 * EncLib.h
 *
 * Created: 4/18/2023 2:09:41 AM
 *  Author: Tyler
 */ 


#ifndef ENCLIB_H_
#define ENCLIB_H_

#define USE_ASCON
#include "CANLib.h"
#include <stdlib.h>
#include <stdint.h>
#include <conf_can.h>
#include <string.h>
#include <asf.h>
#include <stdbool.h>
#ifdef USE_ASCON
#include "ASCON/api.h"
#include "ASCON/ascon.h"
#include "ASCON/crypto_aead.h"
#include "ASCON/constants.h"
#endif
#ifdef USE_PQC
#include "null"
#endif
#include "TreeProtocol/node.h"
//#include "Enrollment.h"
#include "FourQ/FourQ_api.h"
#include "Photon/photon.h"

#define FOURQ_KEY_SIZE 32
#define RESPONSE_SIZE 16

// TODO: Do a server define based on hardware ID?... idk how we're going to store the router's public keys

struct Router_Data {
	uint8_t response_hash[RESPONSE_SIZE];
	uint8_t public_key[FOURQ_KEY_SIZE];
};

struct HWID_Table_Entry {
	struct hardware_id hw_id;
	struct Router_Data router_data;
};

struct ASCON_Data {
	uint8_t session_key[ASCON_128_KEYBYTES];
	volatile uint8_t nonce[CRYPTO_NPUBBYTES];
};

struct Crypto_Data {
	uint8_t secret_key[FOURQ_KEY_SIZE];
	uint8_t public_key[FOURQ_KEY_SIZE];
	uint8_t shared_secret[FOURQ_KEY_SIZE];
	uint8_t shared_hash[ASCON_128_KEYBYTES];
	struct ASCON_Data ASCON_data;
	struct ASCON_Data child_ASCON_data;
	//uint8_t session_key[ASCON_128_KEYBYTES];
	//volatile uint8_t nonce[CRYPTO_NPUBBYTES];
};

extern struct Crypto_Data selfData;
extern struct HWID_Table_Entry HWIDTable[NODE_TOTAL+1];
extern struct HWID_Table_Entry * parentStoredKeys;
extern const char* myname;

// Use the node type to generate the values, we can treat the nodes as the following hardware_ids:
// Pick the node in CANLIB.h
#define ROUTER_0A_HWID		1
#define ROUTER_1A_HWID		2
#define ROUTER_1B_HWID		3
#define EP_2A_MOTOR_HWID	4
#define EP_2A_FAN_HWID		5
#define EP_2A_TSENS_HWID	6
#define EP_2B_MOTOR_HWID	7
#define EP_2B_FAN_HWID		8
#define EP_2B_TSENS_HWID	9		

/*		--------1--------
	----2----		----3----
	4	5	6		7	8	9

^ this is the structure of our design, for example
*/

// Function Prototypes


// Special secret function here
int InitKWIDKeyData();
int authToParent(struct node* my_parent_info, struct node* parent_info, struct node* parent_broadcast_info);

#endif /* ENCLIB_H_ */