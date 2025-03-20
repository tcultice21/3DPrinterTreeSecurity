/*
 * CANLib.h
 *
 * Created: 4/23/2023 12:22:48 AM
 *  Author: Marshmallow
 */ 


#ifndef CANLIB_H_
#define CANLIB_H_

#include <stdlib.h>
#include <stdint.h>
#include <conf_can.h>
#include <string.h>
#include <asf.h>
#include <stdbool.h>

extern char* node_my_name;
#define debug_print(str,...) printf("%s: " str "\r",node_my_name,##__VA_ARGS__)

#define NETWORK_MAX_BUFFER_ELEMENTS 8
#define NODE_TOTAL	3 // K

// Special Reserved Addresses
#define PARENT_NODE_ID 0
#define BROADCAST_ID (uint16_t)-1U

///// PICK ONE /////
//    #define ROUTER_0A
//    #define SYSTEM_ROOT_ROUTER
////////////////////
//  #define ROUTER_1A
//  #define SYSTEM_ROUTER_TYPE
////////////////////
// #define ROUTER_1B
// #define SYSTEM_ROUTER_TYPE
////////////////////
// #define EP_2A_MOTOR
// #define SYSTEM_ENDPOINT_TYPE
// #define SYSTEM_NODE_TYPE 2
////////////////////
// #define EP_2A_FAN
// #define SYSTEM_ENDPOINT_TYPE
// #define SYSTEM_NODE_TYPE 3
////////////////////
// #define EP_2A_TSENS
// #define SYSTEM_ENDPOINT_TYPE
// #define SYSTEM_NODE_TYPE 4
////////////////////
// #define EP_2B_MOTOR
// #define SYSTEM_ENDPOINT_TYPE
// #define SYSTEM_NODE_TYPE 2
////////////////////
// #define EP_2B_FAN
// #define SYSTEM_ENDPOINT_TYPE
// #define SYSTEM_NODE_TYPE 3
////////////////////
#define EP_2B_TSENS
#define SYSTEM_ENDPOINT_TYPE
#define SYSTEM_NODE_TYPE 4
/////////////////////////////////


#if defined(SYSTEM_ENDPOINT_TYPE)
#define NETWORK_MAX_BUFFS 2
#define EXPECTED_NUM_CHILDREN	0
#define EXPECTED_NUM_SIBLINGS	4
#define CAN0_BROADCAST_INDEX	1 // Buffer for broadcast traffic
#define CAN0_COMM_BUFF_INDEX	0 // Buffer for all normal traffic
#elif defined(SYSTEM_ROUTER_TYPE)
#define NETWORK_MAX_BUFFS 3
#define EXPECTED_NUM_CHILDREN	4
#define EXPECTED_NUM_SIBLINGS	3
#define CAN0_COMM_BUFF_INDEX	0 // Use for Server Rx=0 and Tx'ing
#define CAN1_BROADCAST_INDEX	1// Buffer for broadcast traffic
#define CAN1_COMM_BUFF_INDEX	2// Buffer for all normal traffic
#elif defined(SYSTEM_ROOT_ROUTER)
#define NETWORK_MAX_BUFFS 1
#define EXPECTED_NUM_CHILDREN	3
#define EXPECTED_NUM_SIBLINGS	0
#define CAN0_COMM_BUFF_INDEX    0 // Use for Server Rx=0 and Tx'ing
#else
#error "Did not select a node type."
#include <stophere>
#endif

struct multiBuffer {
	uint8_t last_write;
	uint8_t last_read;
	struct can_rx_element_buffer buffers[NETWORK_MAX_BUFFER_ELEMENTS];
} typedef multiBuffer;

struct network {
	struct can_module * can_instance;
	struct multiBuffer * rx_element_buff;
	unsigned int buff_num;
	unsigned int broadcast_buff_num;
};

struct hardware_id {
	uint32_t data;
};

struct network_addr {
	uint16_t CAN_addr;
};

extern struct can_module can0_instance;
extern struct multiBuffer CAN0_rx_element_buff[NETWORK_MAX_BUFFS];

#ifdef SYSTEM_ROUTER_TYPE
extern struct can_module can1_instance;
extern struct multiBuffer CAN1_rx_element_buff[NETWORK_MAX_BUFFS];
#endif

extern const uint32_t HARDWARE_ID_VAL;

// Rx Functions
uint8_t CAN_Rx_Disable(int bufferNum, struct can_module * can_inst);
uint8_t * CAN_Rx(uint16_t idVal, int bufferNum, struct can_module * can_inst);
uint8_t *CAN_Rx_FIFO(uint16_t idVal, uint16_t maskVal, uint16_t filterVal, struct can_module * can_inst);

// Tx Functions
int CAN_Tx_Raw(uint16_t idVal, struct can_tx_element * tx_element, uint32_t dataLen, int buffer, struct can_module * can_inst);
//int CAN_Tx_Raw(uint16_t idVal, struct can_tx_element tx_element, uint32_t dataLen, int buffer, struct can_module * can_inst);
int CAN_Tx(uint16_t idVal, uint8_t *data, uint32_t dataLen, int buffer, struct can_module * can_inst);
////////////////////////////////////////////////////////////////////////

// Usable network functions
int network_check_any(struct network* network, struct network_addr* source, uint8_t* buff, size_t len);
int network_start_listening(struct network* network, struct network_addr* selfAddr);
void network_send(struct network* network, struct network_addr* addr, uint8_t* data, size_t len);


inline void network_addr_make_parent(struct network* network, struct network_addr* addr) {
	(void)network;
	addr->CAN_addr = PARENT_NODE_ID;
}

inline void network_addr_make_broadcast(struct network* network, struct network_addr* addr) {
	(void)network;
	addr->CAN_addr = BROADCAST_ID;
}

////////////////////////////////////////////////////////////////////////
inline static void CAN_Tx_Wait(int buffer, struct can_module * can_inst) {
	while(!(can_tx_get_transmission_status(can_inst) & (1 << buffer)));
}

inline int DLC_to_Val(int input) {
	switch(input) {
		case CAN_TX_ELEMENT_T1_DLC_DATA12_Val:
		return 12;
		case CAN_TX_ELEMENT_T1_DLC_DATA16_Val:
		return 16;
		case CAN_TX_ELEMENT_T1_DLC_DATA20_Val:
		return 20;
		case CAN_TX_ELEMENT_T1_DLC_DATA24_Val:
		return 24;
		case CAN_TX_ELEMENT_T1_DLC_DATA32_Val:
		return 32;
		case CAN_TX_ELEMENT_T1_DLC_DATA48_Val:
		return 48;
		case CAN_TX_ELEMENT_T1_DLC_DATA64_Val:
		return 64;
		default:
		printf("What did you just give me.\r\n");
		return 0;
	}
}

inline struct can_rx_element_buffer * getNextBufferElement(struct multiBuffer * buff) {
	if (buff->last_write == buff->last_read) return NULL;
	register int last_element = buff->last_read;
	
	buff->last_read = (buff->last_read + 1) % NETWORK_MAX_BUFFER_ELEMENTS;
	
	return &(buff->buffers[last_element]);
}

// Legacy Defines in case they are needed
//! [can_filter_setting]
/*
#define CAN_RX_STANDARD_FILTER_INDEX_0    0
#define CAN_RX_STANDARD_FILTER_INDEX_1    1
#define CAN_RX_STANDARD_FILTER_ID_0     0x45A
#define CAN_RX_STANDARD_FILTER_ID_0_BUFFER_INDEX     2
#define CAN_RX_STANDARD_FILTER_ID_1     0x469
#define CAN_RX_EXTENDED_FILTER_INDEX_0    0
#define CAN_RX_EXTENDED_FILTER_INDEX_1    1
#define CAN_RX_EXTENDED_FILTER_ID_0     0x100000A5
#define CAN_RX_EXTENDED_FILTER_ID_0_BUFFER_INDEX     1
#define CAN_RX_EXTENDED_FILTER_ID_1     0x10000096
//! [can_filter_setting]

#define CAN_FILTER_MONITOR 1
#define CAN_FILTER_WAIT 2
#define CAN_FILTER_MSG1 3
#define CAN_FILTER_MSG2 4

// [ENCRYPTION STUFF]
#define DEBUGCODE	0
#define NODE_ID		1
#define NODE_TOTAL	2
#define TOTAL_SENDS	10
*/

#endif /* CANLIB_H_ */