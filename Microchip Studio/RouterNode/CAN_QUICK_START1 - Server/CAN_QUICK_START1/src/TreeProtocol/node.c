#include "node.h"
#include "CANLib.h"


char* node_typename_none      = "none";
char* node_typename_router    = "router";
char* node_typename_fan		= "fan";
char* node_typename_motor	= "motor";
char* node_typename_tsens	= "tsens";

char* node_type_to_typename(uint32_t node_type) {
	switch (node_type) {
		case NODE_TYPE_NONE:
		return node_typename_none;
		case NODE_TYPE_ROUTER:
		return node_typename_router;
		case NODE_TYPE_FAN:
		return node_typename_fan;
		case NODE_TYPE_MOTOR:
		return node_typename_motor;
		case NODE_TYPE_TSENS:
		return node_typename_tsens;
	}
}

// cmd is a command
// Returns the appropriate packet length for a given command
uint8_t node_msg_cmd_to_len(uint8_t cmd) {
	switch (cmd) {
	case NODE_CMD_NOP:
		return sizeof(struct node_msg_header);
	case NODE_CMD_DISC_INIT:
		return sizeof(struct node_msg_header) + sizeof(struct hardware_id);
	case NODE_CMD_DISC_RESP:
		return sizeof(struct node_msg_header) + sizeof(struct hardware_id) + sizeof(uint8_t);
	case NODE_CMD_DISC_ASGN:
		return sizeof(struct node_msg_header) + sizeof(struct hardware_id) + sizeof(uint16_t);
	case NODE_CMD_AUTH_PLN:
		return sizeof(struct node_msg_generic);
	case NODE_CMD_AUTH_ENC:
		return sizeof(struct node_msg_generic);
	case NODE_CMD_CAP:
	case NODE_CMD_CAP_GNAD:
		return sizeof(struct node_msg_header) + sizeof(uint32_t);
	case NODE_CMD_HID:
	case NODE_CMD_HID_GNAD:
		return sizeof(struct node_msg_header) + sizeof(struct hardware_id);
	case NODE_CMD_NAD:
	case NODE_CMD_NAD_GCAP:
		return sizeof(struct node_msg_header) + sizeof(struct node_addr);
	case NODE_CMD_READ:
	case NODE_CMD_WRITE_RESP:
		return sizeof(struct node_msg_header) + sizeof(uint32_t);
	case NODE_CMD_WRITE:
	case NODE_CMD_READ_RESP:
		return sizeof(struct node_msg_header) + sizeof(uint32_t) + sizeof(uint32_t);
	default:
		return 0;
	}
}

// header is header to fill address into
// addr is address to put into header
void node_msg_addr_fill(struct node_msg_header* header, struct node_addr* addr) {
	header->addr = (uint16_t) -1U;
	for (int i = 0; i < addr->len; i++) {
		header->addr = (header->addr << 4) | (addr->hops[i] & 0xf);
	}
}

void node_msg_ret_addr_fill(struct node_msg_header* header, struct node_addr* ret_addr) {
	header->ret = ret_addr->hops[0]; // by convention, first is local addr
}

// header is header struct to compare address with
// addr is addr to compare with
// Returns 1 if addresses are equivalent, 0 otherwise
int node_msg_addr_eq(struct node_msg_header* header, struct node_addr* addr) {
	struct node_msg_header temp_header;
	node_msg_addr_fill(&temp_header, addr);
	return (header->addr == temp_header.addr);
}

void node_make_parent(struct network* network, struct node* parent) {
	network_addr_make_parent(network, &parent->haddr);
	parent->addr.hops[0] = 0;
	parent->addr.len = 1;
	memset((char*)&parent->hid, 0, sizeof(parent->hid));
	parent->type = NODE_TYPE_ROUTER;
	parent->network = network;
}

void node_make_broadcast(struct network* network, struct node* broadcast) {
	network_addr_make_broadcast(network, &broadcast->haddr);
	broadcast->addr.hops[0] = (uint16_t) -1U;
	broadcast->addr.len = 1;
	memset((char*)&broadcast->hid, 0, sizeof(broadcast->hid));
	broadcast->type = NODE_TYPE_NONE;
	broadcast->network = network;
}

void node_make_remote_from_router(struct network* network, struct node* router, struct node* remote, struct node_addr* addr) {
	int i = 0;
	for (; i < 4 && i < router->addr.len; i++) {
		remote->addr.hops[i] = router->addr.hops[i];
	}
	for (int j = 0; j < 4 && i < 4 && j < addr->len; j++,i++) {
		remote->addr.hops[i] = addr->hops[j];
	}
	remote->addr.len = i;
	debug_print("Addr %x:%x:%x:%x, l:%d\n", remote->addr.hops[0], remote->addr.hops[1], remote->addr.hops[2], remote->addr.hops[3], remote->addr.len);
	memset((char*)&remote->hid, 0, sizeof(remote->hid));
	memmove(&remote->haddr, &router->haddr, sizeof(remote->haddr));
	remote->type = NODE_TYPE_NONE;
	remote->network = network;
	remote->encryption_data = router->encryption_data;
}

void node_make_source_from_msg(struct network* network, struct node* source, struct node* node_table, int num_nodes, void* msg) {
	struct node_msg_generic* msg_generic = msg;
	int addr_len = 0;
	for (; addr_len < 4; addr_len++) {
		debug_print("addr[%d] = %d\n", addr_len, ((msg_generic->header.addr >> (addr_len * 4)) & 0xf));
		if (((msg_generic->header.addr >> (addr_len * 4)) & 0xf) == 0xf) {
			break;
		}
	}
	if (addr_len == 0) {
		addr_len = 1;
	}
	for (int i = addr_len - 1; i >= 0; i--) {
		source->addr.hops[i] = (msg_generic->header.addr >> (i * 4)) & 0xf;
		debug_print("hops[%d] = %d\n", i, source->addr.hops[i]);
	}
	source->addr.len = addr_len;
	memset((char*)&source->hid, 0, sizeof(source->hid));
	memmove(&source->haddr, &node_table[msg_generic->header.ret].haddr, sizeof(source->haddr));
	source->type = NODE_TYPE_NONE;
	source->network = network;
}

uint8_t node_msg_cmd_encrypted(uint8_t cmd) {
	switch (cmd) {
		case NODE_CMD_NOP:
		return 0;
		case NODE_CMD_DISC_INIT:
		return 0;
		case NODE_CMD_DISC_RESP:
		return 0;
		case NODE_CMD_DISC_ASGN:
		return 0;
		case NODE_CMD_AUTH_PLN:
		return 0;
		case NODE_CMD_AUTH_ENC:
		return 1;
		case NODE_CMD_CAP:
		case NODE_CMD_CAP_GNAD:
		return 1;
		case NODE_CMD_HID:
		case NODE_CMD_HID_GNAD:
		return 1;
		case NODE_CMD_NAD:
		case NODE_CMD_NAD_GCAP:
		return 1;
		case NODE_CMD_READ:
		case NODE_CMD_WRITE_RESP:
		return 1;
		case NODE_CMD_WRITE:
		case NODE_CMD_READ_RESP:
		return 1;
		default:
		return 0;
	}
}

// self is own node info (may not use this later on, we'll see)
// cmd is the node command type of the message
// destination is destination node
// msg is message to send
void node_msg_send(struct node* self, struct node* destination, enum node_cmd cmd, void* msg) {
	struct node_msg_generic* msg_generic = msg;
	node_msg_addr_fill(&msg_generic->header, &destination->addr);
	debug_print("Sending to naddr %x, len %d\n", *((uint16_t*)&msg_generic->header.addr), destination->addr.len);
	msg_generic->header.counter = rand(); // need to figure out how to configure this
	msg_generic->header.TTL = destination->addr.len - 1;
	node_msg_ret_addr_fill(&msg_generic->header, &self->addr);
	msg_generic->header.cmd = cmd;
	msg_generic->header.len = node_msg_cmd_to_len(cmd);
	if(node_msg_cmd_encrypted(cmd)) {
		uint8_t buff[64];
		*((uint32_t *)&self->encryption_data->nonce[0]) += msg_generic->header.counter;
		uint32_t tempLen[2] = {0,0};
		uint32_t tempInLen[2] = {((msg_generic->header.len-sizeof(struct node_msg_header)+7)/8)*8,0};
		crypto_aead_encrypt(&buff[sizeof(struct node_msg_header)],&tempLen[0],msg_generic->data,tempInLen[0],NULL,NULL,NULL,self->encryption_data->nonce,self->encryption_data->session_key);
		*((uint32_t *)&self->encryption_data->nonce[0]) -= msg_generic->header.counter;
		memcpy(buff,&msg_generic->header,sizeof(struct node_msg_header));
		network_send(destination->network, &destination->haddr, (uint8_t*)buff, tempLen[0]+sizeof(struct node_msg_header));
		return;
	}
	/*debug_print("Sending: ");
	for (int i = 0; i < 8; i++) {
		printf("%x ", ((uint8_t*)msg)[i]);
	}
	printf("\n");*/
	network_send(destination->network, &destination->haddr, (uint8_t*)msg_generic, msg_generic->header.len);
}

// self is own node info (may not use this later on, we'll see)
// cmd is the node command type of the message
// destination is destination node
// msg is message to send
// len is the message length
void node_msg_send_generic(struct node* self, struct node* destination, enum node_cmd cmd, void* msg, uint8_t len) {
	len += sizeof(struct node_msg_header);
	struct node_msg_generic* msg_generic = msg;
	int proper_len = (len < sizeof(struct node_msg_generic)) ? len : sizeof(struct node_msg_generic);
	node_msg_addr_fill(&msg_generic->header, &destination->addr);
	debug_print("Sending to naddr %x, len %d\n", *((uint16_t*)&msg_generic->header.addr), destination->addr.len);
	msg_generic->header.counter = 0;
	msg_generic->header.TTL = destination->addr.len - 1;
	node_msg_ret_addr_fill(&msg_generic->header, &self->addr);
	msg_generic->header.cmd = cmd;
	msg_generic->header.len = proper_len;
	// dest should be broadcast
	if(node_msg_cmd_encrypted(cmd)) {
		uint8_t buff[64];
		*((uint32_t *)&self->encryption_data->nonce[0]) += msg_generic->header.counter;
		uint32_t tempLen[2] = {0,0};
		uint32_t tempInLen[2] = {((msg_generic->header.len-sizeof(struct node_msg_header)+7)/8)*8,0};
		crypto_aead_encrypt(&buff[sizeof(struct node_msg_header)],&tempLen[0],msg_generic->data,tempInLen[0],NULL,NULL,NULL,self->encryption_data->nonce,self->encryption_data->session_key);
		*((uint32_t *)&self->encryption_data->nonce[0]) -= msg_generic->header.counter;
		memcpy(buff,&msg_generic->header,sizeof(struct node_msg_header));
		network_send(destination->network, &destination->haddr, (uint8_t*)buff, tempLen[0]+sizeof(struct node_msg_header));
		return;
	}
	network_send(destination->network, &destination->haddr, (uint8_t*)msg_generic, proper_len);
}

// node_table is the node table for the destination network
// self is self address in destination network
// msg is the message to forward (MODIFIED BY THIS FUNCTION)
void node_msg_forward(struct node* node_table, int num_nodes, struct node* self, void* msg) {
	struct node_msg_generic* msg_generic = msg;
	if (msg_generic->header.TTL < 1) {
		return;
	}
	msg_generic->header.TTL -= 1;

	uint16_t next_naddr = (msg_generic->header.addr >> (msg_generic->header.TTL * 4)) & 0xf;
	if (next_naddr >= num_nodes) {
		debug_print("naddr too large %d\n", next_naddr);
		return;
	}
	int proper_len = (msg_generic->header.len < sizeof(struct node_msg_generic)) ? msg_generic->header.len : sizeof(struct node_msg_generic);
	struct node* destination = &node_table[next_naddr];
	msg_generic->header.counter = 0;
	debug_print("Forwarding message from %d to %d\n", msg_generic->header.ret, destination->addr.hops[0]);
	node_msg_ret_addr_fill(&msg_generic->header, &self->addr);
	if(node_msg_cmd_encrypted(msg_generic->header.cmd)) {
		uint8_t buff[64];
		*((uint32_t *)&self->encryption_data->nonce[0]) += msg_generic->header.counter;
		uint32_t tempLen[2] = {0,0};
		uint32_t tempInLen[2] = {((msg_generic->header.len-sizeof(struct node_msg_header)+7)/8)*8,0};
		crypto_aead_encrypt(&buff[sizeof(struct node_msg_header)],&tempLen[0],msg_generic->data,tempInLen[0],NULL,NULL,NULL,self->encryption_data->nonce,self->encryption_data->session_key);
		*((uint32_t *)&self->encryption_data->nonce[0]) -= msg_generic->header.counter;
		
		memcpy(buff,&msg_generic->header,sizeof(struct node_msg_header));
		network_send(destination->network, &destination->haddr, (uint8_t*)buff, tempLen[0]+sizeof(struct node_msg_header));
		return;
	}
	network_send(destination->network, &destination->haddr, (uint8_t*)msg_generic, sizeof(struct node_msg_generic));
}

// source is the node to check for a message from
// msg_buff is the buffer to write the received message to (if any)
void node_msg_wait(struct node* self, void* msg_buff) {
	struct node_msg_generic* msg_generic = msg_buff;
	int okay;
	int received;
	do {
		okay = 1;
		do {
			received = network_check_any(self->network, NULL, msg_buff, sizeof(struct node_msg_generic));
		} while (received == 0);
		uint32_t unpaddedVal = ((msg_generic->header.len-sizeof(struct node_msg_header)+7)/8)*8+8;
		if(node_msg_cmd_encrypted(msg_generic->header.cmd)) {
			uint8_t buff[56];
			*((uint32_t *)&self->encryption_data->nonce[0]) += msg_generic->header.counter;
			uint32_t tempLen[2] = {0,0};
			if (crypto_aead_decrypt(buff,&tempLen[0],NULL,msg_generic->data,unpaddedVal,NULL,NULL,self->encryption_data->nonce,self->encryption_data->session_key)) {
				debug_print("Decrypt Tag Failed %i, %i\n",unpaddedVal,tempLen[0]);
				debug_print("Received: \n");
				for (int i = 0; i < tempLen[0]; i++) {
					printf("%x ", buff[sizeof(struct node_msg_header)+i]);
				}
				printf("\r\n");
				debug_print("Session Key: \n");
				for (int i = 0; i < 16; i++) {
					printf("%x ",self->encryption_data->session_key[i]);
				}
				printf("\r\n");
				debug_print("Nonce: \n");
				for (int i = 0; i < 16; i++) {
					printf("%x ",self->encryption_data->nonce[i]);
				}
				printf("\r\n");
				*((uint32_t *)&self->encryption_data->nonce[0]) -= msg_generic->header.counter;
				okay = 0;
			}
			
			*((uint32_t *)&self->encryption_data->nonce[0]) -= msg_generic->header.counter;
			memcpy(&msg_buff[sizeof(struct node_msg_header)],buff,tempLen[0]);
		}
	} while (!okay);
	int shift = msg_generic->header.TTL * 4;
	msg_generic->header.addr = (msg_generic->header.addr & ~(0xf << shift)) | (msg_generic->header.ret << shift);
}

// source is the 
// msg_buff is the buffer to write the received message to (if any)
void node_msg_wait_any_assign(struct node* source, void* msg_buff) {
	struct node_msg_generic* msg_generic = msg_buff;
	int received;
	do {
		received = network_check_any(source->network, &source->haddr, msg_buff, sizeof(struct node_msg_generic));
	} while (received == 0);
	if(node_msg_cmd_encrypted(msg_generic->header.cmd)) {
		uint8_t buff[56];
		*((uint32_t *)&source->encryption_data->nonce[0]) += msg_generic->header.counter;
		//debug_print("Nonce Value given: %i\n",*((uint32_t *)&source->encryption_data->nonce[0]));
		uint32_t tempLen[2] = {0,0};
		/*debug_print("Received: \n");
		for (int i = 0; i < received-sizeof(struct node_msg_header); i++) {
			printf("%x ", msg_generic->data[i]);
		}*/
		printf("\r\n");
		if (crypto_aead_decrypt(buff,&tempLen[0],NULL,msg_generic->data,received-sizeof(struct node_msg_header),NULL,NULL,source->encryption_data->nonce,source->encryption_data->session_key)) {
			debug_print("Decrypt Tag Failed %i, %i\n",received-sizeof(struct node_msg_header),tempLen[0]);
			debug_print("Received: \n");
			for (int i = 0; i < tempLen[0]; i++) {
				printf("%x ", buff[sizeof(struct node_msg_header)+i]);
			}
			printf("\r\n");
			debug_print("Session Key: \n");
			for (int i = 0; i < 16; i++) {
				printf("%x ",source->encryption_data->session_key[i]);
			}
			printf("\r\n");
			debug_print("Nonce: \n");
			for (int i = 0; i < 16; i++) {
				printf("%x ",source->encryption_data->nonce[i]);
			}
			printf("\r\n");
			*((uint32_t *)&source->encryption_data->nonce[0]) -= msg_generic->header.counter;
			return;
		}
		
		*((uint32_t *)&source->encryption_data->nonce[0]) -= msg_generic->header.counter;
		memcpy(&msg_buff[sizeof(struct node_msg_header)],buff,tempLen[0]);
	}
	int shift = msg_generic->header.TTL * 4;
	msg_generic->header.addr = (msg_generic->header.addr & ~(0xf << shift)) | (msg_generic->header.ret << shift);
	//debug_print("node_msg_wait %d\n", received);
}

// source is the node to check for a message from
// msg_buff is the buffer to write the received message to (if any)
// Returns: the length written if a message is available, 0 otherwise
int node_msg_check(struct node* source, void* msg_buff) {
	int written = network_check_any(source->network, NULL, msg_buff, sizeof(struct node_msg_generic));
	if (written > 0) {
		struct node_msg_generic* msg_generic = msg_buff;
		uint32_t unpaddedVal = ((msg_generic->header.len-sizeof(struct node_msg_header)+7)/8)*8+8;
		if(node_msg_cmd_encrypted(msg_generic->header.cmd)) {
			uint8_t buff[56];
			*((uint32_t *)&source->encryption_data->nonce[0]) += msg_generic->header.counter;
			uint32_t tempLen[2] = {0,0};
			if (crypto_aead_decrypt(buff,&tempLen[0],NULL,msg_generic->data,unpaddedVal,NULL,NULL,source->encryption_data->nonce,source->encryption_data->session_key)) {
				debug_print("Decrypt Tag Failed %i, %i\n",unpaddedVal,tempLen[0]);
				debug_print("Received: \n");
				for (int i = 0; i < tempLen[0]; i++) {
					printf("%x ", buff[sizeof(struct node_msg_header)+i]);
				}
				printf("\r\n");
				debug_print("Session Key: \n");
				for (int i = 0; i < 16; i++) {
					printf("%x ",source->encryption_data->session_key[i]);
				}
				printf("\r\n");
				debug_print("Nonce: \n");
				for (int i = 0; i < 16; i++) {
					printf("%x ",source->encryption_data->nonce[i]);
				}
				printf("\r\n");
				*((uint32_t *)&source->encryption_data->nonce[0]) -= msg_generic->header.counter;
				return 0;
			}
			
			*((uint32_t *)&source->encryption_data->nonce[0]) -= msg_generic->header.counter;
			memcpy(&msg_buff[sizeof(struct node_msg_header)],buff,tempLen[0]);
		}
		int shift = msg_generic->header.TTL * 4;
		msg_generic->header.addr = (msg_generic->header.addr & ~(0xf << shift)) | (msg_generic->header.ret << shift);
		//debug_print("Receiving from naddr %x, ret %x\n", *((uint16_t*)&msg_generic->header.addr), msg_generic->header.ret);
		return unpaddedVal;
	}
	return 0;
}


/*void node_msg_buffer_init(struct node_msg_buffer* buffer, struct node_msg_buffer_element* elements, int num_elements, struct node* node_table) {
	if (num_elements <= 0) return;
	buffer->arr = elements;
	buffer->free_head = &elements[0];
	buffer->queue_front = NULL;
	buffer->queue_back = NULL;
	buffer->node_table = node_table;
	buffer->len = num_elements;

	for (int i = 0; i < num_elements-1; i++) {
		elements[i].queue_next = &elements[i + 1];
		elements[i].node_next = NULL;
	}
	elements[num_elements - 1].queue_next = NULL;//&elements[0];
	elements[num_elements - 1].node_next = NULL;

	elements[0].queue_prev = NULL;//&elements[num_elements - 1];
	elements[0].node_prev = NULL;
	for (int i = 1; i < num_elements; i++) {
		elements[i].queue_prev = NULL;//&elements[i - 1];
		elements[i].node_prev = NULL;
	}
}

struct node_msg_buffer_element* node_msg_buffer_new(struct node_msg_buffer* buffer) {
	struct node_msg_buffer_element* element;
	if (buffer->free_head != NULL) {
		element = buffer->free_head;
		buffer->free_head = element->queue_next;
	}
	else if (buffer->queue_front != NULL) {
		element = buffer->queue_front;
		buffer->queue_front = element->queue_next;
		// always NULL
		//if (element->queue_prev != NULL) element->queue_prev->queue_next = element->queue_next;
		if (element->queue_next != NULL) element->queue_next->queue_prev = element->queue_prev;
		// always NULL
		//if (element->node_prev != NULL) element->node_prev->node_next = element->node_next;
		if (element->node_next != NULL) element->node_next->node_prev = element->node_prev;
	}
	else {
		return NULL;
	}

	// can remove these after debugging is complete
	element->queue_next = NULL;
	//element->queue_prev = NULL;
	element->node_next = NULL;
	//element->node_prev = NULL;

	return element;
}

void node_msg_buffer_insert(struct node_msg_buffer* buffer, struct node_msg_buffer_element* element) {
	struct node_msg_buffer_element* curr_queue_back = buffer->queue_back;
	element->node_prev = curr_queue_back;
	element->node_next = NULL; //curr_queue_back->node_prev;
	if (curr_queue_back != NULL) curr_queue_back->node_next = element;
	buffer->queue_back = element;

	struct node_msg_buffer_element* curr_node_back = buffer->node_table[element->msg.header.ret].msg_queue_back;
	element->node_prev = curr_node_back;
	element->node_next = NULL; //curr_node_back->node_prev;
	if (curr_node_back != NULL) curr_node_back->node_next = element;
	buffer->node_table[element->msg.header.ret].msg_queue_back = element;
}

void node_msg_buffer_free(struct node_msg_buffer* buffer, struct node_msg_buffer_element* element) {
	element->queue_next = buffer->free_head;
	buffer->free_head = element;

	// can remove these after debugging is complete
	element->queue_prev = NULL;
	element->node_next = NULL;
	element->node_prev = NULL;
}*/