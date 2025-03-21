/*
 * EncLIB.c
 *
 * Created: 4/18/2023 2:08:23 AM
 *  Author: Tyler
 */ 
#include "EncLib.h"

///*
#define CANINTERFACE 1
//#ifdef(CANINTERFACE)
#define TEMP_NODE_ID 1
#define TEMP_FILTER_VAL 1

struct Crypto_Data selfData = {.ASCON_data = {.nonce = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15}}, .child_ASCON_data = {.nonce = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15}}};

int InitKWIDKeyData() {
	// Takes the table of HWIDTable and initializes all the values
	uint8_t secret_key[32];
	//Create your secret key.
	memset(selfData.secret_key,HARDWARE_ID_VAL,32);
	
	// How the routers do it:
	#if defined(SYSTEM_ROUTER_TYPE) || defined(SYSTEM_ROOT_ROUTER)
		for(int i = 0; i < NODE_TOTAL+1; i++) {
			
			ECCRYPTO_STATUS Status;
			if(HWIDTable[i].hw_id.data == 0) continue;
			
			// Make up its secret key; this will be how the node does it too.
			memset(secret_key,(uint8_t)(HWIDTable[i].hw_id.data),sizeof(secret_key));
			memset(HWIDTable[i].router_data.public_key,0,32);
			
			// Generate the public key (this technically shouldn't be done on the OTHER nodes lol)
			Status = CompressedKeyGeneration(secret_key,HWIDTable[i].router_data.public_key);
			
			// Generate its expected response hash
			photon128(secret_key,32,HWIDTable[i].router_data.response_hash);
		}
	#endif
	#ifdef SYSTEM_ENDPOINT_TYPE
		// Generate router's public key... even though you shouldn't be doing this.
		ECCRYPTO_STATUS Status;
		
		memset(secret_key,(uint8_t)(parentStoredKeys->hw_id.data),sizeof(secret_key));
		memset(parentStoredKeys->router_data.public_key,0,32);
		
		// Generate the public key (this technically shouldn't be done on the OTHER nodes lol)
		Status = CompressedKeyGeneration(secret_key,parentStoredKeys->router_data.public_key);
		
		// Generate its expected response hash
		photon128(secret_key,32,parentStoredKeys->router_data.response_hash);
	#endif
}

// WHEN COPYING DO NOT OVERWRITE THE OTHER SIDE WITH THIS.
int authToParent(struct node* my_parent_info, struct node* parent_info, struct node* parent_broadcast_info) {
	struct node_msg_generic message;
	uint8_t response[16];
	unsigned long mlen, clen;
	
	/*
	Marshmallow ? Yesterday at 8:06 PM
	1) Server will request a node to send its information
	2) If the node is a router:
	- It sends an encrypted PUF hash using a key made of stored parent/child public key data (they should both pre-store it), if it works the server will send its hash too. if not, it'll ignore it.
	If the node is a... node:
	- It sends its public key to the server then generates the shared key hash then
	3) The server will send an encrypted PUF hash to the child, the end node will verify it with their recorded hash to validate that the parent is legitimate
	4) The node sends an OK to server
	5) Server sends the session key and nonce to the device after cleared
	AFTER ALL NODES: An All clear is sent and is used to sync the devices for continuing further
	I presumed it would do this process linearly for each node, because I don't want to try to make "pseudo parallelism" in this work
	And there would be a "timeout" present for each step on the server's side
	If the timeout occurs, the node doesn't get called "validated"
	And it doesn't get to have communication spoken to it, nor does it get the session key
	*/
	
	
	ECCRYPTO_STATUS Status = CompressedKeyGeneration(selfData.secret_key,selfData.public_key);
	if (Status != ECCRYPTO_SUCCESS) {
		printf("Failed Public Key Generation\r\n");
		return Status;
	}
	
	// The server's public key should be stored so use it to make your shared key
	Status = CompressedSecretAgreement(selfData.secret_key,parentStoredKeys->router_data.public_key,selfData.shared_secret);
	if (Status != ECCRYPTO_SUCCESS) {
		printf("Failed Shared Secret Creation\r\n");
		return Status;
	}
	photon128(selfData.shared_secret,32,selfData.shared_hash);

	// Server will request a node to send its information, this data is pretty much unnecessary
	// Wait for it.
	while(node_msg_check(parent_info, &message) == 0 || message.header.cmd != NODE_CMD_AUTH_PLN);
	//We are a router, thus we handle auth differently
	
	// Send public key to Server
	memcpy(message.data,selfData.public_key,FOURQ_KEY_SIZE);

	node_msg_send_generic(my_parent_info,parent_info,NODE_CMD_AUTH_PLN,&message,FOURQ_KEY_SIZE);
	// If this auth fails, the server will not be sending anything else and we will be stuck.
	
	// Server will then share its encrypted hash to verify the other way around too.
	while(node_msg_check(parent_info, &message) == 0 || message.header.cmd != NODE_CMD_AUTH_PLN);

	crypto_aead_decrypt(response,&mlen,NULL,message.data,message.header.len-8,NULL,NULL,selfData.ASCON_data.nonce,selfData.shared_hash);
	if (memcmp(response,parentStoredKeys->router_data.response_hash,mlen) != 0) {
		// TODO: Find out if this PUF response matches when encrypted
		exit(2);
	}
	
	// If OK, send an OK back, if not just ignore them and fault
	memset(message.data,1,8);
	node_msg_send_generic(my_parent_info,parent_info,NODE_CMD_AUTH_PLN,&message,8);
	
	// Server will send the session key back
	while(node_msg_check(my_parent_info, &message) == 0 || message.header.cmd != NODE_CMD_AUTH_PLN);
	if (crypto_aead_decrypt(response,&mlen,NULL,message.data,message.header.len-8,NULL,NULL,selfData.ASCON_data.nonce,selfData.shared_hash)) {
		debug_print("Signature for Session Key was Incorrect!\n");
		exit(2);
	}
	memcpy(selfData.ASCON_data.session_key,response,16);
	
	// Receive broadcast saying init is over, time for GO (because you are a router, your next step will be to do the server-side yourself)
	while(node_msg_check(parent_broadcast_info, &message) == 0);
	debug_print("Completed Parent's Authentication\n");
}

// Preallocated tables of hardware IDs to public keys are:
	#if defined(ROUTER_0A)
		const uint32_t HARDWARE_ID_VAL = ROUTER_0A_HWID;
		const char* myname = "ROUTER_0A";
		struct HWID_Table_Entry HWIDTable[NODE_TOTAL+1] = {
			{
				.hw_id = {.data = 0} // No Parent - 0
			},
			{
				.hw_id = {.data = ROUTER_1A_HWID} // Child 1 - Router 1A
			},
			{
				.hw_id = {.data = ROUTER_1B_HWID} // Child 2 - Router 2B
			},
			{
				.hw_id = {.data = 0} // No Child 3
			}
		};
		struct HWID_Table_Entry * parentStoredKeys = &HWIDTable[0];
	#elif defined(ROUTER_1A)
		const uint32_t HARDWARE_ID_VAL = ROUTER_1A_HWID;
		const char* myname = "ROUTER_1A";
		struct HWID_Table_Entry HWIDTable[NODE_TOTAL+1] = {
			{
				.hw_id = {.data = ROUTER_0A_HWID} // Parent - Router 0A
			},
			{
				.hw_id = {.data = EP_2A_MOTOR_HWID} // Child 1 - Device A's Motor
			},
			{
				.hw_id = {.data = EP_2A_FAN_HWID} // Child 2 - Device A's Fan
			},
			{
				.hw_id = {.data = EP_2A_TSENS_HWID} // Child 3 - Device
			}
		};
		struct HWID_Table_Entry * parentStoredKeys = &HWIDTable[0];
	#elif defined(ROUTER_1B)
		const uint32_t HARDWARE_ID_VAL = ROUTER_1B_HWID;
		const char* myname = "ROUTER_1B";
		struct HWID_Table_Entry HWIDTable[NODE_TOTAL+1] = {
			{
				.hw_id = {.data = ROUTER_0A_HWID} // Parent - Router 0A
			},
			{
				.hw_id = {.data = EP_2A_MOTOR_HWID} // Child 1 - Device A's Motor
			},
			{
				.hw_id = {.data = EP_2A_FAN_HWID} // Child 2 - Device A's Fan
			},
			{
				.hw_id = {.data = EP_2A_TSENS_HWID} // Child 3 - Device
			}
		};
		struct HWID_Table_Entry * parentStoredKeys = &HWIDTable[0];
	#elif defined(EP_2A_FAN)
		const uint32_t HARDWARE_ID_VAL = EP_2A_FAN_HWID;
		const char* myname = "EP_2A_FAN";
		struct HWID_Table_Entry HWIDParent = {
				.hw_id = {.data = ROUTER_1A_HWID}
			};
		struct HWID_Table_Entry * parentStoredKeys = &HWIDParent;
	#elif defined(EP_2A_MOTOR)
		const uint32_t HARDWARE_ID_VAL = EP_2A_MOTOR_HWID;
		const char* myname = "EP_2A_MOTOR";
		struct HWID_Table_Entry HWIDParent = {
			.hw_id = {.data = ROUTER_1A_HWID}
		};
		struct HWID_Table_Entry * parentStoredKeys = &HWIDParent;
	#elif defined(EP_2A_TSENS)
		const uint32_t HARDWARE_ID_VAL = EP_2A_TSENS_HWID;
		const char* myname = "EP_2A_TSENS";
		struct HWID_Table_Entry HWIDParent = {
			.hw_id = {.data = ROUTER_1A_HWID}
		};
		struct HWID_Table_Entry * parentStoredKeys = &HWIDParent;
	#elif defined(EP_2B_FAN)
		#define SYSTEM_NODE_TYPE NODE_TYPE_FAN
		const uint32_t HARDWARE_ID_VAL = EP_2B_FAN_HWID;
		const char* myname = "EP_2B_FAN";
		struct HWID_Table_Entry HWIDParent = {
			.hw_id = {.data = ROUTER_1B_HWID}
		};
		struct HWID_Table_Entry * parentStoredKeys = &HWIDParent;
	#elif defined(EP_2B_MOTOR)
		#define SYSTEM_NODE_TYPE NODE_TYPE_MOTOR
		const uint32_t HARDWARE_ID_VAL = EP_2B_MOTOR_HWID;
		const char* myname = "EP_2B_MOTOR";
		struct HWID_Table_Entry HWIDParent = {
			.hw_id = {.data = ROUTER_1B_HWID}
		};
		struct HWID_Table_Entry * parentStoredKeys = &HWIDParent;
	#elif defined(EP_2B_TSENS)
		#define SYSTEM_NODE_TYPE NODE_TYPE_TSENS
		const uint32_t HARDWARE_ID_VAL = EP_2B_TSENS_HWID;
		const char* myname = "EP_2B_TSENS";
		struct HWID_Table_Entry HWIDParent = {
			.hw_id = {.data = ROUTER_1B_HWID}
		};
		struct HWID_Table_Entry * parentStoredKeys = &HWIDParent;
	#endif
	
	