/*
 * EncLIB.c
 *
 * Created: 4/18/2023 2:08:23 AM
 *  Author: Marshmallow
 */ 
#include "EncLib.h"

///*
#define CANINTERFACE 1
// Defines used for testing system w/o Joseph's protocol
//#ifdef(CANINTERFACE)
#define TEMP_NODE_ID 1
#define TEMP_FILTER_VAL 1

struct hardware_id {
	uint8_t data[4];
};

struct node_addr {
	int addr;
};

struct node {
	uint16_t addr;
	struct hardware_id hid;
	uint32_t node_type;
};

enum node_cmd {
	NODE_CMD_NOP = 0,
	NODE_CMD_DISC_INIT = 1,
	NODE_CMD_DISC_RESP = 2,
	NODE_CMD_DISC_ASGN = 3,
	NODE_CMD_AUTH_PLN = 4,
	NODE_CMD_AUTH_ENC = 5,
	NODE_CMD_CAP = 6,
	NODE_CMD_HID = 7,
	NODE_CMD_NAD = 8,
	NODE_CMD_CAP_GNAD = 9,
	NODE_CMD_HID_GNAD = 10,
	NODE_CMD_NAD_GCAP = 11,
	NODE_CMD_DATA = 12
};

struct node_msg_header {
	uint16_t addr;
	uint16_t counter;
	uint8_t TTL;
	uint8_t ret;
	uint8_t cmd;
	uint8_t : 8;
};

struct node_msg_generic {
	struct node_msg_header header;
	uint8_t data[48];
	uint8_t len;
};

struct node self = {
	.addr = TEMP_NODE_ID,
	.hid = TEMP_NODE_ID,
	.node_type = 1
	};

struct node server = {
	.addr = 0,
	.hid = 0,
	.node_type = 0
	};

// self is own node info (may not use this later on, we'll see)
// destination is destination node
// msg is message to send
void node_msg_send(struct node* self, struct node* destination, void* msg, struct can_module* can_instance) {
	uint8_t sendMsg[64];
	struct node_msg_generic* message = (struct node_msg_generic *)msg;
	unsigned long clen = message->len;
	message->header.addr = destination->addr;
	if(message->header.cmd == NODE_CMD_AUTH_ENC) {
		crypto_aead_encrypt(&sendMsg[8], &clen, message->data, message->len, NULL, NULL, NULL, selfData.ASCON_data.nonce, selfData.shared_hash);
	}
	else {
		memcpy(&sendMsg[8],message->data,message->len);
		clen = message->len;
	}
	CAN_Tx(destination->addr,msg,clen+sizeof(struct node_msg_header),TEMP_FILTER_VAL,&can_instance);
	CAN_Tx_Wait(TEMP_FILTER_VAL,&can_instance);
}

int node_msg_check(struct node* source, void* msg_buff) {
	struct node_msg_generic structBuf;
	unsigned long mlen;
	
	if (rx_element_buff[TEMP_FILTER_VAL].last_read < rx_element_buff[TEMP_FILTER_VAL].last_write) {
		memcpy(&structBuf,getNextBufferElement(&rx_element_buff[TEMP_FILTER_VAL])->data,((struct node_msg_generic *)(getNextBufferElement(&rx_element_buff[TEMP_FILTER_VAL])->data))->len);
		
		if (structBuf.header.cmd == NODE_CMD_AUTH_ENC) {
			crypto_aead_decrypt(msg_buff, &mlen, (void*)0, &structBuf.data, 24, NULL, NULL, selfData.ASCON_data.nonce, selfData.shared_hash);
		}
		
		else {
			memcpy(msg_buff,&structBuf,structBuf.len);
		}
		return 1;
	}
	
	else {
		return 0;
	}
}

//#endif
//*/

uint8_t ServerPublicKey[32];
struct Crypto_Data selfData = {.ASCON_data = {.nonce = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15}}};

// TODO: This probably doesn't need passed the nodes, just the parent and itself?
int authToParent(void * nodes, struct can_module* can_instance) {
	//uint8_t cryptoBuff[64];
	struct node_msg_generic message;
	uint8_t response[16];
	
	/*
	Marshmallow ? Yesterday at 8:06 PM
	1) Server will request a node to send its information
	2) If the node is a router:
	- It sends an encrypted PUF hash using a key made of stored parent/child public key data (they should both pre-store it)
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
	CAN_Rx(TEMP_NODE_ID,TEMP_FILTER_VAL,can_instance);
	
	ECCRYPTO_STATUS Status = CompressedKeyGeneration(selfData.secret_key,selfData.public_key);
	if (Status != ECCRYPTO_SUCCESS) {
		printf("Failed Public Key Generation\r\n");
		return Status;
	}
	
	// The server's public key should be stored so use it to make your shared key?
	Status = CompressedSecretAgreement(selfData.secret_key,ServerPublicKey,selfData.shared_secret);
	if (Status != ECCRYPTO_SUCCESS) {
		printf("Failed Shared Secret Creation\r\n");
		return Status;
	}
	
	// Server will request a node to send its information, this data is pretty much unnecessary
	// Wait for it.
	while(node_msg_check(&server, &message) == 0);
	
	 // We are a node, thus we do that side of the client auth scheme
	 
	 // Send your public key to the server
	 message.header.cmd = NODE_CMD_AUTH_PLN;
	 message.header.counter = 0;
	 message.header.TTL = 1;
	 message.len = FOURQ_KEY_SIZE;
	 memcpy(message.data,selfData.public_key,FOURQ_KEY_SIZE);
	 node_msg_send(&self, &server, &message, can_instance);
	 
	 // Server will send encrypted PUF has to this node and we will validate it with the one we have stored
	 // Wait for it.
	 while(node_msg_check(&server, &message) == 0);
	 if (memcmp(message.data,serverPUFResponse,16) != 0) {
		 // TODO: Find out if this PUF response matches when encrypted
		 printf("The server is not the one you want to connect to!\r\n");
		 exit(2);
	 }
	 
	 // TODO: If OK, send an OK back, if not just ignore them and fault
	memset(message.data,1,8);
	message.header.cmd = NODE_CMD_AUTH_PLN;
	message.header.counter = 0;
	message.header.TTL = 1;
	message.len = 8;
	node_msg_send(&self, &server, &message, can_instance);
	
	// Server will send the session key back
	while(node_msg_check(&server, &message) == 0);
	memcpy(selfData.ASCON_data.session_key,message.data,16);
	
	// Receive broadcast saying init is over, time for GO
	// TODO
}
