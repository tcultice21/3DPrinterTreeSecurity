#ifndef NODE_H
#define NODE_H


#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "CANLib.h"
#include "EncLib.h"

// by convention, addresses are number from nearest to furthest
struct node_addr {
	int len;
	int hops[4];
};

enum node_type {
	NODE_TYPE_NONE = 0,
	NODE_TYPE_ROUTER = 1,
	NODE_TYPE_MOTOR = 2,
	NODE_TYPE_FAN = 3,
	NODE_TYPE_TSENS = 4
};

struct node_msg_buffer_element;

struct node {
	struct node_addr addr;
	struct hardware_id hid;
	struct network_addr haddr;
	uint32_t type;
	struct ASCON_Data * encryption_data;
	//struct node_msg_buffer_element* msg_queue_front;
	//struct node_msg_buffer_element* msg_queue_back;
	struct network* network;
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
	NODE_CMD_READ = 12,
	NODE_CMD_READ_RESP = 13,
	NODE_CMD_WRITE = 14,
	NODE_CMD_WRITE_RESP = 15
};

struct node_msg_header {
	uint16_t addr;
	uint16_t counter;
	uint8_t TTL;
	uint8_t ret;
	uint8_t cmd;
	uint8_t len;
};

// can add an "extended message header" which adds more command space for commands with little data

struct node_msg_generic {
	struct node_msg_header header;
	uint8_t data[48];
};

struct node_msg_discover_init {
	struct node_msg_header header;
	// addr is broadcast
	// counter is probably 0?
	// TTL is 1
	// ret is my_id (which will always be 0 here)
	// cmd is NODE_CMD_DISC_INIT
	union {
		struct {
			struct hardware_id hid;
			// a value which is different for all devices
		};
		uint8_t data[48];
	};
};

struct node_msg_discover_response {
	struct node_msg_header header;
	// addr is 0
	// counter is ???
	// TTL is 1
	// ret is broadcast
	// cmd is NODE_CMD_DISC_RESP
	union {
		struct {
			struct hardware_id hid;
			// a value which is different for all devices
			uint8_t capability;
			// capability contains the first/primary capability of the node
			// if the node wants to be authenticated as a router, this must be 0
		};
		uint8_t data[48];
	};
};

struct node_msg_discover_assign {
	struct node_msg_header header;
	// addr is broadcast
	// counter is ???
	// TTL is 1
	// ret is my_id (which will always be 0 here)
	// cmd is NODE_CMD_DISC_ASGN
	union {
		struct {
			struct hardware_id hid;
			// value from node_msg_discover_response
			uint16_t addr;
			// the assigned addr of the child node
		};
		uint8_t data[48];
	};
};

/*struct node_msg_discover_key {
	struct node_msg_header header;
	// addr is broadcast
	// counter is ???
	// TTL is 1
	// ret is 0
	// cmd is NODE_CMD_DISC_KEY
	union {
		struct {
			struct hardware_id hid;
			// the same hid as the response
			crypt_key(FourQ) key;
			// an asymmetric encryption key
		};
		uint8_t data[48];
	};
	uint8_t len;
};*/

struct node_msg_cap {
	struct node_msg_header header;
	// addr is broadcast
	// counter is ???
	// TTL is 1
	// ret is my_id (which will always be 0 here)
	// cmd is NODE_CMD_CAP or NODE_CMD_CAP_GNAD
	union {
		struct {
			uint32_t cap;
			// a capability
		};
		uint8_t data[48];
	};
};

struct node_msg_hid {
	struct node_msg_header header;
	// addr is broadcast
	// counter is ???
	// TTL is 1
	// ret is my_id (which will always be 0 here)
	// cmd is NODE_CMD_HID or NODE_CMD_HID_GNAD
	union {
		struct {
			struct hardware_id hid;
			// a hardware ID
		};
		uint8_t data[48];
	};
};

struct node_msg_nodeaddr {
	struct node_msg_header header;
	// addr is broadcast
	// counter is ???
	// TTL is 1
	// ret is my_id (which will always be 0 here)
	// cmd is NODE_CMD_NAD or NODE_CMD_NAD_GCAP
	union {
		struct {
			struct node_addr addr;
			// a node address
		};
		uint8_t data[48];
	};
};

struct node_msg_data_addr {
	struct node_msg_header header;
	// addr is broadcast
	// counter is ???
	// TTL is 1
	// ret is my_id (which will always be 0 here)
	// cmd is NODE_CMD_READ or NODE_CMD_WRITE_RESP
	union {
		struct {
			uint32_t addr;
			// a node address
		};
		uint8_t data[48];
	};
};

struct node_msg_data_addr_value {
	struct node_msg_header header;
	// addr is broadcast
	// counter is ???
	// TTL is 1
	// ret is my_id (which will always be 0 here)
	// cmd is NODE_CMD_WRITE or NODE_CMD_READ_RESP
	union {
		struct {
			uint32_t addr;
			uint32_t value;
			// a node address
		};
		uint8_t data[48];
	};
};


/*struct node_msg_buffer_element {
	struct node_msg_generic msg;
	struct node_msg_buffer_element* queue_prev; // also functions as free list pointer
	struct node_msg_buffer_element* queue_next; // also functions as free list pointer
	struct node_msg_buffer_element* node_prev;
	struct node_msg_buffer_element* node_next;
};

struct node_msg_buffer {
	struct node_msg_buffer_element* queue_front;
	struct node_msg_buffer_element* queue_back;
	struct node_msg_buffer_element* free_head;
	struct node* node_table;
	struct node_msg_buffer_element* arr;
	int len;
};*/


char* node_type_to_typename(uint32_t node_type);

void node_make_parent(struct network* network, struct node* parent);

void node_make_broadcast(struct network* network, struct node* broadcast);

void node_make_remote_from_router(struct network* network, struct node* router, struct node* remote, struct node_addr* addr);

void node_make_source_from_msg(struct network* network, struct node* source, struct node* node_table, int num_nodes, void* msg);

// self is own node info (may not use this later on, we'll see)
// cmd is the node command type of the message
// destination is destination node
// msg is message to send
void node_msg_send(struct node* self, struct node* destination, enum node_cmd cmd, void* msg);

// self is own node info (may not use this later on, we'll see)
// cmd is the node command type of the message
// destination is destination node
// msg is message to send
// len is the message length
void node_msg_send_generic(struct node* self, struct node* destination, enum node_cmd cmd, void* msg, uint8_t len);

// node_table is the node table for the destination network
// self is self address in destination network
// msg is the message to forward (MODIFIED BY THIS FUNCTION)
void node_msg_forward(struct node* node_table, int num_nodes, struct node* self, void* msg);

// source is the node to check for a message from
// msg_buff is the buffer to write the received message to (if any)
void node_msg_wait(struct node* source, void* msg_buff);

// source is the node to check for a message from
// msg_buff is the buffer to write the received message to (if any)
void node_msg_wait_any_assign(struct node* source, void* msg_buff);

// source is the node to check for a message from
// msg_buff is the buffer to write the received message to (if any)
// Returns: 1 if a message is available (and writes to the buffer if not NULL) and 0 otherwise
int node_msg_check(struct node* source, void* msg_buff);

#endif