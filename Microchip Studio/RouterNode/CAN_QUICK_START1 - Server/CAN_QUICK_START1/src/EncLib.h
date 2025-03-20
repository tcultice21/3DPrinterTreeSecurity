/*
 * EncLib.h
 *
 * Created: 4/23/2023 12:20:18 AM
 *  Author: Marshmallow
 */ 


#ifndef ENCLIB_H_
#define ENCLIB_H_

#include "CANLib.h"
#include <stdlib.h>
#include <stdint.h>
#include <conf_can.h>
#include <string.h>
#include <asf.h>
#include <stdbool.h>

// IMPORTANT!!! Select One:
//#define USE_ASCON 1
#define USE_PQC 1


//#ifdef USE_ASCON
#include "ASCON/api.h"
#include "ASCON/ascon.h"
#include "ASCON/crypto_aead.h"
#include "ASCON/constants.h"
#include "FourQ/FourQ_api.h"
#include "Photon/photon.h"
//#endif

#ifdef USE_PQC
#include "PQCEnc/api.h"
#include "PQCEnc/aes.h"
#include "PQCEnc/symmetric.h"
#include "PQCEnc/aes.h"
#endif

#include "TreeProtocol/node.h"

//#include "Enrollment.h"


#define FOURQ_KEY_SIZE 32
#define RESPONSE_SIZE 16


struct Router_Data {
	#ifdef USE_ASCON
	uint8_t response_hash[RESPONSE_SIZE];
	uint8_t public_key[FOURQ_KEY_SIZE];
	#endif
	#ifdef USE_PQC
	uint8_t response_hash[SHA3_256_RATE];
	#endif
};

struct HWID_Table_Entry {
	struct hardware_id hw_id;
	struct Router_Data router_data;
};

// Information needed for post-quantum cryptography
struct AES_Data {
	uint8_t session_key[AES256_KEYBYTES];
	volatile uint8_t nonce[CRYPTO_NPUBBYTES];
};

struct ASCON_Data {
	uint8_t session_key[ASCON_128_KEYBYTES];
	volatile uint8_t nonce[CRYPTO_NPUBBYTES];
};

struct Crypto_Data {
	#ifdef USE_ASCON
		uint8_t secret_key[FOURQ_KEY_SIZE];
		uint8_t public_key[FOURQ_KEY_SIZE];
		uint8_t shared_secret[FOURQ_KEY_SIZE];
		uint8_t shared_hash[ASCON_128_KEYBYTES];
		struct ASCON_Data ASCON_data;
		struct ASCON_Data child_ASCON_data;
	#endif
	#ifdef USE_PQC
		uint8_t secret_key[PQCLEAN_KYBER512_CLEAN_CRYPTO_SECRETKEYBYTES];
		uint8_t public_key[PQCLEAN_KYBER512_CLEAN_CRYPTO_PUBLICKEYBYTES];
		uint8_t parent_public_key[PQCLEAN_KYBER512_CLEAN_CRYPTO_PUBLICKEYBYTES];
		uint8_t shared_secret[AES256_KEYBYTES];
		struct AES_Data ASCON_data;
		struct AES_Data child_ASCON_data;
	#endif
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
int authToChildren(struct node * nodes, int nodeLen, struct node* my_child_info, struct node* child_broadcast_info);
int authToParent(struct node* my_parent_info, struct node* parent_info, struct node* parent_broadcast_info);

#endif /* ENCLIB_H_ */