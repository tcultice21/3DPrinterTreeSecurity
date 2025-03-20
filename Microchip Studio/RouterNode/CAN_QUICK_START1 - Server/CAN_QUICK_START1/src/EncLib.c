/*
 * EncLib.c
 *
 * Created: 4/23/2023 12:19:45 AM
 *  Author: Marshmallow
 */ 

#include "EncLib.h"

///*
#define CANINTERFACE 1
//#ifdef(CANINTERFACE)
#define TEMP_NODE_ID 1
#define TEMP_FILTER_VAL 1
#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define FLATTEN(x) ((x > 48) ? 48 : (x))


#ifdef USE_ASCON
struct Crypto_Data selfData = {.ASCON_data = {.nonce = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15}}, .child_ASCON_data = {.nonce = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15}}};

int InitKWIDKeyData() {
	// Takes the table of HWIDTable and initializes all the values (because we don't want to have to hand copy them all...
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
		uint8_t secret_key[32];
		ECCRYPTO_STATUS Status;
		
		memset(secret_key,(uint8_t)(parentStoredKeys->hw_id.data),sizeof(secret_key));
		memset(parentStoredKeys->router_data.public_key,0,32);
		
		// Generate the public key (this technically shouldn't be done on the OTHER nodes lol)
		Status = CompressedKeyGeneration(secret_key,parentStoredKeys->router_data.public_key);
		
		// Generate its expected response hash
		photon128(secret_key,32,parentStoredKeys->router_data.response_hash);
	#endif
}
#endif

#ifdef USE_PQC
struct Crypto_Data selfData = {.ASCON_data = {.nonce = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15}}, .child_ASCON_data = {.nonce = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15}}};
int InitKWIDKeyData() {
	// Takes the table of HWIDTable and initializes all the values (because we don't want to have to hand copy them all...
	
	// How the routers do it:
	#if defined(SYSTEM_ROUTER_TYPE) || defined(SYSTEM_ROOT_ROUTER)
	
	// Generate the keys here.
	PQCLEAN_KYBER512_CLEAN_crypto_kem_keypair(selfData.public_key,selfData.secret_key);
	memcpy(selfData.parent_public_key,selfData.public_key,PQCLEAN_KYBER512_CLEAN_CRYPTO_PUBLICKEYBYTES);
	hash_h(parentStoredKeys->router_data.response_hash,selfData.parent_public_key,PQCLEAN_KYBER512_CLEAN_CRYPTO_PUBLICKEYBYTES);
	
	for(int i = 1; i < NODE_TOTAL+1; i++) {
		
		// To save time (this isn't timed/evaluated) we'll just copy over all the responses here.
		if(HWIDTable[i].hw_id.data == 0) continue;
		memcpy(HWIDTable[i].router_data.response_hash,parentStoredKeys->router_data.response_hash,SHA3_256_RATE);
	}
	#endif
	#ifdef SYSTEM_ENDPOINT_TYPE
	// Generate router's public key... Think of as an "establishment phase"
	
	// Copy the keys over because the RNG generator is the same regardless.
	// Only doing this to simplify the keygen process. This should be changed in production code.
	// This makes no impact on the runtime performance of the algorithm, as we isolate this keygen from test results anyway.
	PQCLEAN_KYBER512_CLEAN_crypto_kem_keypair(selfData.parent_public_key,selfData.secret_key);
	memcpy(selfData.public_key,selfData.parent_public_key,PQCLEAN_KYBER512_CLEAN_CRYPTO_PUBLICKEYBYTES);
	
	// Generate its expected response hash
	hash_h(parentStoredKeys->router_data.response_hash,selfData.parent_public_key,PQCLEAN_KYBER512_CLEAN_CRYPTO_PUBLICKEYBYTES);
	//photon128(secret_key,32,parentStoredKeys->router_data.response_hash);
	#endif
}
#endif

// TODO: This probably doesn't need passed the nodes, just the parent and itself?
#ifdef USE_ASCON
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
	
	// (For my purpose only)
	// Enable monitoring
	//CAN_Rx(TEMP_NODE_ID,TEMP_FILTER_VAL,can_instance);
	
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
	
	//debug_print("Waiting for our time in Auth.\r\n");
	// Server will request a node to send its information, this data is pretty much unnecessary
	// Wait for it.
	while(node_msg_check(parent_info, &message) == 0 || message.header.cmd != NODE_CMD_AUTH_PLN);
	//debug_print("Our turn in Auth.\r\n");
	//We are a router, thus we handle auth differently
	
	// Send encrypted response to server
	photon128(selfData.secret_key,32,response);
	crypto_aead_encrypt(message.data,&clen,response,RESPONSE_SIZE,NULL,NULL,NULL,selfData.ASCON_data.nonce,selfData.shared_hash);
	//debug_print("Clen = %i\n",clen);
	node_msg_send_generic(my_parent_info,parent_info,NODE_CMD_AUTH_PLN,&message,clen);
	// If this auth fails, the server will not be sending anything else and we will be stuck.
	
	// Server will then share its encrypted hash to verify the other way around too.
	while(node_msg_check(parent_info, &message) == 0 || message.header.cmd != NODE_CMD_AUTH_PLN);
	/*debug_print("Enc Response: \n");
	for (int sex = 0; sex < message.header.len-8; sex++) {
		printf("%x ",message.data[sex]);
	}
	printf("\r\n");*/
	crypto_aead_decrypt(response,&mlen,NULL,message.data,message.header.len-8,NULL,NULL,selfData.ASCON_data.nonce,selfData.shared_hash);
	if (memcmp(response,parentStoredKeys->router_data.response_hash,16) != 0) {
		// TODO: Find out if this PUF response matches when encrypted
		printf("The server is not the one you want to connect to!\r\n");
		debug_print("Expected Response: \n");
		for (int sex = 0; sex < RESPONSE_SIZE; sex++) {
			printf("%x ",parentStoredKeys->router_data.response_hash[sex]);
		}
		printf("\r\n");
		debug_print("Received Response: \n");
		for (int sex = 0; sex < RESPONSE_SIZE; sex++) {
			printf("%x ",response[sex]);
		}
		printf("\r\n");
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
	/*debug_print("Generated Session Key: \n");
	for (int sex = 0; sex < RESPONSE_SIZE; sex++) {
		printf("%x ",selfData.ASCON_data.session_key[sex]);
	}
	printf("\r\n");*/
	
	// Receive broadcast saying init is over, time for GO (because you are a router, your next step will be to do the server-side yourself)
	while(node_msg_check(parent_broadcast_info, &message) == 0);
	debug_print("Completed Parent's Authentication\n");
}
#endif

#ifdef USE_PQC
int authToParent(struct node* my_parent_info, struct node* parent_info, struct node* parent_broadcast_info) {
	return 0;
}
#endif

// ASCON-based Authentication to Children
#ifdef USE_ASCON
int authToChildren(struct node * nodes, int nodeLen, struct node* my_child_info, struct node* child_broadcast_info) {
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
	
	// I won't be implementing a timeout on this. If they're not all there, I don't want it.
	ECCRYPTO_STATUS Status;
	
	// For each child node in the list.
	for(int i = 1; i < nodeLen; i++) {
		
		// Gather context information about HID's device.
		uint32_t hw_id = nodes[i].hid.data;
		struct HWID_Table_Entry * thisNode = NULL;
		for(int j = 1; j < NODE_TOTAL+1; j++) {
			if(hw_id == HWIDTable[j].hw_id.data) {
				thisNode = &HWIDTable[j];
				debug_print("Node %i: Found on entry %i w/ HW_ID %i (%i)\n",i,j,hw_id,HWIDTable[j].hw_id.data);
				break;
			}
		}
		if (thisNode == NULL) {
			printf("Warning: Found device HID %i not registered in HWID system.\r\n",hw_id);
			continue;
		}
		
		// use hardware ID to generate shared key
		Status = CompressedSecretAgreement(selfData.secret_key,thisNode->router_data.public_key,selfData.shared_secret);
		if (Status != ECCRYPTO_SUCCESS) {
			printf("Warning: Secret Agreement Failed with Node HID %i\r\n",hw_id);
			continue;
		}
		photon128(selfData.shared_secret,32,selfData.shared_hash);
		
		// Request a node to send it's information.
		// TODO: Send out a packet req
		memset(message.data,1,12);
		node_msg_send_generic(my_child_info,&nodes[i],NODE_CMD_AUTH_PLN,&message,12);
		
		// Node type is router
		if (nodes[i].type == NODE_TYPE_ROUTER) { 
			uint8_t * expectedResponse = thisNode->router_data.response_hash;
			// use hardware ID to acquire the PUF response
			
			// Child Router will send encrypted hash, take it and check it
			// TODO: Wait and accept a packet
			while(node_msg_check(&nodes[i], &message) == 0 || message.header.cmd != NODE_CMD_AUTH_PLN);
			if(message.header.len != sizeof(response)+8) {
				printf("Error: HID %i sent an incorrect sized packet.\r\n",thisNode->hw_id.data);
			}
			
			crypto_aead_decrypt(response,&mlen,NULL,message.data,message.header.len-8,NULL,NULL,selfData.ASCON_data.nonce,selfData.shared_hash);
			if(memcmp(response,thisNode->router_data.response_hash,16) != 0) {
				printf("Warning: Node HID %i failed the response test.\r\n",hw_id);
				debug_print("Expected Response: \n");
				for (int j = 0; j < RESPONSE_SIZE; j++) {
					printf("%x ",thisNode->router_data.response_hash[j]);
				}
				printf("\r\n");
				debug_print("Received Response: \n");
				for (int j = 0; j < RESPONSE_SIZE; j++) {
					printf("%x ",response[j]);
				}
				printf("\r\n");
				continue;
			}
			
		}
		// Node type is a endpoint
		else if (nodes[i].type != NODE_TYPE_NONE) { 
			// Nodes will send their public key... we kinda already have them so ignore it I guess.

			while(node_msg_check(&nodes[i], &message) == 0 || message.header.cmd != NODE_CMD_AUTH_PLN);
			if(message.header.len-8 != FOURQ_KEY_SIZE) {
				printf("Error: HID %i sent an incorrect sized packet.\r\n",thisNode->hw_id.data);
			}
			if(memcmp(message.data,thisNode->router_data.public_key,FOURQ_KEY_SIZE) != 0) {
				printf("Warning: Node HID %i sent a public key that doesn't match!\r\n",hw_id);
				debug_print("Expected Public Key: \n");
				for (int sex = 0; sex < FOURQ_KEY_SIZE; sex++) {
					printf("%x ",thisNode->router_data.public_key[sex]);
				}
				printf("\r\n");
				debug_print("Received Public Key: \n");
				for (int sex = 0; sex < FOURQ_KEY_SIZE; sex++) {
					printf("%x ",message.data[sex]);
				}
				printf("\r\n");
				continue;
			}
		}
		else {
			printf("Warning: Node HID %i registered as type None.\r\n",hw_id);
			continue;
		}
		
		// Send the node your response hash
		photon128(selfData.secret_key,32,response);
		crypto_aead_encrypt(message.data,&clen,response,RESPONSE_SIZE,NULL,NULL,NULL,selfData.ASCON_data.nonce,selfData.shared_hash);
		node_msg_send_generic(my_child_info,&nodes[i],NODE_CMD_AUTH_PLN,&message,clen);
		
		// Node will reply with an OK message if things validated
		while(node_msg_check(&nodes[i], &message) == 0 || message.header.cmd != NODE_CMD_AUTH_PLN);
		
		// Generate a session key and distribute the session key back
		memset(selfData.child_ASCON_data.session_key,HARDWARE_ID_VAL,ASCON_128_KEYBYTES);
		crypto_aead_encrypt(message.data,&clen,selfData.child_ASCON_data.session_key,ASCON_128_KEYBYTES,NULL,NULL,NULL,selfData.ASCON_data.nonce,selfData.shared_hash);
		node_msg_send_generic(my_child_info,&nodes[i],NODE_CMD_AUTH_PLN,&message,clen);
	}
	//Send broadcast saying init is over, and it's time to go.
	memset(message.data,1,12);
	node_msg_send_generic(my_child_info,child_broadcast_info,NODE_CMD_AUTH_PLN,&message,12);
	debug_print("Completed Our Authentication\n");
}
#endif

#ifdef USE_PQC
int authToChildren(struct node * nodes, int nodeLen, struct node* my_child_info, struct node* child_broadcast_info) {
	struct node_msg_generic message;
	uint8_t response[PQCLEAN_KYBER512_CLEAN_CRYPTO_CIPHERTEXTBYTES+PQCLEAN_KYBER512_CLEAN_CRYPTO_PUBLICKEYBYTES];
	uint8_t decrypt[64];
	// This node's public key is stored in selfData.
	// We need a place to store the child's public key.
	// I won't be implementing a timeout on this. If they're not all there, I don't want it.
	
	/*
	KEM-based Authentication (Router to Router):
	1) Child sends Public Key to Server alongside encapsulated challenge 1 using Server's Public Key
	2) Server hashes Child's Public Key, Checks to it's known value to ensure it matches (is this necessary, we should use)
	3) Encapsulates a random key value and transmits the challenge 2 to the Child
	4) Decrypted keys are XOR'ed together, establishing a shared key made from both parties (should this be more rigid? Hashing maybe? Look into it perhaps)
	5) A challenge-response is created that will validate the authenticity of both sides using XOR'ed shared key w/ AES
	- If either device sends an invalid response, neither side should accept the other
	- Child starts first (See next step and it'll explain why)
	- Server's response will also contain a 32-byte sequence of the session key that will be used for local subnet communication

	KEM-based Authentication (Router to Device)(No Device Authentication Needed):
	1) Child sends encapsulated challenge 1 using Server's Public Key (is child public key even needed here?)
	2) Server decrypts and uses encapsulated challenge as session key
	3) Server transmits a known cipher text using AES w/ the session key decapsulated from step 1 (Also could just decapsulate and send I guess?)
	4) Child validates the server's authenticity by validating that the ciphertext was correct
	5) Server transmits 32-bytes to child containing the session key used for that local subnet communication (either in same message or different one)
	*/
	ECCRYPTO_STATUS Status;
	// Create the overall session key NOW
	memset(selfData.child_ASCON_data.session_key,HARDWARE_ID_VAL,AES256_KEYBYTES);
	
	// For each child node in the list.
	for(int i = 1; i < nodeLen; i++) {
		
		// Gather context information about HID's device.
		uint32_t hw_id = nodes[i].hid.data;
		struct HWID_Table_Entry * thisNode = NULL;
		for(int j = 1; j < NODE_TOTAL+1; j++) {
			if(hw_id == HWIDTable[j].hw_id.data) {
				thisNode = &HWIDTable[j];
				debug_print("Node %i: Found on entry %i w/ HW_ID %i (%i)\n",i,j,hw_id,HWIDTable[j].hw_id.data);
				break;
			}
		}
		if (thisNode == NULL) {
			printf("Warning: Found device HID %i not registered in HWID system.\r\n",hw_id);
			continue;
		}
		
		// Tell child it's their turn.
		memset(message.data,1,12);
		node_msg_send_generic(my_child_info,&nodes[i],NODE_CMD_AUTH_PLN,&message,12);
		
		// Node type is router
		if (nodes[i].type == NODE_TYPE_ROUTER) {
			// Child will send public key to Server alongside encapsulated challenge 1 using Server's Public Key.
			for(int j = 0; j < PQCLEAN_KYBER512_CLEAN_CRYPTO_PUBLICKEYBYTES+PQCLEAN_KYBER512_CLEAN_CRYPTO_CIPHERTEXTBYTES; j += 48) {
				while(node_msg_check(&nodes[i], &message) == 0 || message.header.cmd != NODE_CMD_AUTH_PLN);
				memcpy(&response[j],message.data,FLATTEN(PQCLEAN_KYBER512_CLEAN_CRYPTO_CIPHERTEXTBYTES+PQCLEAN_KYBER512_CLEAN_CRYPTO_PUBLICKEYBYTES-j));//MAX((PQCLEAN_KYBER512_CLEAN_CRYPTO_CIPHERTEXTBYTES+PQCLEAN_KYBER512_CLEAN_CRYPTO_PUBLICKEYBYTES-j)%48,48));
			}
		
			// Server hashes Child's Public Key, Checks to it's known value to ensure it matches
			// Can probably just hash using SHA256.
			uint8_t hashOutput[SHA3_256_RATE];
			hash_h(hashOutput,response,PQCLEAN_KYBER512_CLEAN_CRYPTO_PUBLICKEYBYTES);
			if(memcmp(hashOutput,thisNode->router_data.response_hash,SHA3_256_RATE)) {
				// Comparison of hashes failed. This Public key is not correct.
				debug_print("Failed comparison of hashes on HID device %i.\r",hw_id);
				continue;
			}
		
			// Decrypt and store message if everything goes okay here.
			PQCLEAN_KYBER512_CLEAN_crypto_kem_dec(decrypt,&response[PQCLEAN_KYBER512_CLEAN_CRYPTO_PUBLICKEYBYTES],selfData.secret_key);
			
			//Encapsulates a random key value and transmits the challenge 2 to the Child
			for(int j = 0; j < AES256_KEYBYTES; j++) {
				selfData.shared_secret[j] = hw_id;
			}
			PQCLEAN_KYBER512_CLEAN_crypto_kem_enc(&response[PQCLEAN_KYBER512_CLEAN_CRYPTO_PUBLICKEYBYTES],selfData.shared_secret,&response[0]);
			
			// Transmit it back to the Child.
			for(int j = 0; j < PQCLEAN_KYBER512_CLEAN_CRYPTO_CIPHERTEXTBYTES; j+=48) {
				memcpy(message.data,&response[PQCLEAN_KYBER512_CLEAN_CRYPTO_PUBLICKEYBYTES+j],FLATTEN(PQCLEAN_KYBER512_CLEAN_CRYPTO_CIPHERTEXTBYTES-j));
				node_msg_send_generic(my_child_info,&nodes[i],NODE_CMD_AUTH_PLN,&message,FLATTEN(PQCLEAN_KYBER512_CLEAN_CRYPTO_CIPHERTEXTBYTES-j));
			}
			
			// XOR the keys generated by both sides together
			for(int j = 0; j < AES256_KEYBYTES; j++) {
				selfData.shared_secret[j] ^= decrypt[j];
			}
			
			// receive the challenge
			while(node_msg_check(&nodes[i], &message) == 0 || message.header.cmd != NODE_CMD_AUTH_PLN);
			// CTR mode is reversible, use that AES, remember to init, maybe make wrapper function(?), make sure to stop adding the counter to the IV (its its own thing now), use it separately.
			// TODO Above.
			// Decrypt
			
			// Return the challenge, return a session key to use
			// Return enc(16B-challenge || session_key) = 48B
			memcpy(message.data,decrypt,16);
			memcpy(&message.data[16],selfData.child_ASCON_data.session_key,AES256_KEYBYTES);
			node_msg_send_generic(my_child_info,&nodes[i],NODE_CMD_AUTH_PLN,&message,48);
			
			// Done?
			
		}
		// Node type is a endpoint
		else if (nodes[i].type != NODE_TYPE_NONE) {
			/*
			KEM-based Authentication (Router to Device)(No Device Authentication Needed):
			1) Child sends encapsulated challenge 1 using Server's Public Key (is child public key even needed here, no?)
			2) Server decrypts and uses encapsulated challenge as session key
			3) Server transmits a known cipher text using AES w/ the session key decapsulated from step 1 (Also could just decapsulate and send I guess?)
			4) Child validates the server's authenticity by validating that the ciphertext was correct
			5) Server transmits 32-bytes to child containing the session key used for that local subnet communication (either in same message or different one)
			*/	
		
			// Skipping Child Sends public key, just encapsulated challenge
			// Receive the encapsulated challenge 1
			for(int j = 0; j < PQCLEAN_KYBER512_CLEAN_CRYPTO_CIPHERTEXTBYTES; j += 48) {
				while(node_msg_check(&nodes[i], &message) == 0 || message.header.cmd != NODE_CMD_AUTH_PLN);
				memcpy(&response[j],message.data,FLATTEN(PQCLEAN_KYBER512_CLEAN_CRYPTO_CIPHERTEXTBYTES-j));
			}
			
			// Server decrypts and uses encapsulated challenge as challenge key
			// using the space for &response[CIPHERTEXTBYTES] as buffer space
			PQCLEAN_KYBER512_CLEAN_crypto_kem_dec(&response[PQCLEAN_KYBER512_CLEAN_CRYPTO_CIPHERTEXTBYTES],&response[0],selfData.secret_key);
			
			// Transmit back enc(challenge[0:15] || session_key)
			memcpy(decrypt,&response[PQCLEAN_KYBER512_CLEAN_CRYPTO_CIPHERTEXTBYTES],16);
			memcpy(&decrypt[16],selfData.child_ASCON_data.session_key,AES256_KEYBYTES);
			// AES Encrypt 48 bytes of information
			
			node_msg_send_generic(my_child_info,&nodes[i],NODE_CMD_AUTH_PLN,&message,48);
			
			//If the child accepts the server, he will use the session key provided.
			// If not, he "should" error out and say something to us (when we handle non-static number of nodes, sure)
			
		}
		else {
			printf("Warning: Node HID %i registered as type None.\r\n",hw_id);
			continue;
		}
	}
	
	//Send broadcast saying init is over, and it's time to go.
	memset(message.data,1,12);
	node_msg_send_generic(my_child_info,child_broadcast_info,NODE_CMD_AUTH_PLN,&message,12);
	debug_print("Completed Our Authentication\n");
};
#endif

/* struct HWID_Table_Entry {
struct hardware_id;
struct Router_Data;
}; 

struct hardware_id {
	uint32_t data;
};
*/

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
				.hw_id = {.data = EP_2B_MOTOR_HWID} // Child 1 - Device B's Motor
			},
			{
				.hw_id = {.data = EP_2B_FAN_HWID} // Child 2 - Device B's Fan
			},
			{
				.hw_id = {.data = EP_2B_TSENS_HWID} // Child 3 - Device
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
		const uint32_t HARDWARE_ID_VAL = EP_2B_FAN_HWID;
		const char* myname = "EP_2B_FAN";
		struct HWID_Table_Entry HWIDParent = {
			.hw_id = {.data = ROUTER_1B_HWID}
		};
		struct HWID_Table_Entry * parentStoredKeys = &HWIDParent;
	#elif defined(EP_2B_MOTOR)
		const uint32_t HARDWARE_ID_VAL = EP_2B_MOTOR_HWID;
		const char* myname = "EP_2B_MOTOR";
		struct HWID_Table_Entry HWIDParent = {
			.hw_id = {.data = ROUTER_1B_HWID}
		};
		struct HWID_Table_Entry * parentStoredKeys = &HWIDParent;
	#elif defined(EP_2B_TSENS)
		const uint32_t HARDWARE_ID_VAL = EP_2B_TSENS_HWID;
		const char* myname = "EP_2B_TSENS";
		struct HWID_Table_Entry HWIDParent = {
			.hw_id = {.data = ROUTER_1B_HWID}
		};
		struct HWID_Table_Entry * parentStoredKeys = &HWIDParent;
	#endif