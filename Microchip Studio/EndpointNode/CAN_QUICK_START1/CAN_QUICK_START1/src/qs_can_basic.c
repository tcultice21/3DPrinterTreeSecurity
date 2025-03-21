/**
 * \file
 *
 * Based on SAM CAN basic Quick Start
 * Copyright (c) 2015-2018 Microchip Technology Inc. and its subsidiaries.
 *
 * Author: Tyler
 *
 */
/*
 * Support and FAQ: visit <a href="https://www.microchip.com/support/">Microchip Support</a>
 */
#include <asf.h>
#include <string.h>
#include <conf_can.h>
#include "CANLib.h"
#include "EncLib.h"

// Peripherals
#include "EPPeripherals.h"
//! [module_var]

//! [module_inst]
static struct usart_module cdc_instance;
static struct can_module can_instance;

//extern dev_cap_structure cap_callback_ref;
//! [module_inst]

//! [module_var]

//! [cdc_setup]
static void configure_usart_cdc(void)
{

	struct usart_config config_cdc;
	usart_get_config_defaults(&config_cdc);
	config_cdc.baudrate	 = 115200;
	config_cdc.mux_setting = EDBG_CDC_SERCOM_MUX_SETTING;
	config_cdc.pinmux_pad0 = EDBG_CDC_SERCOM_PINMUX_PAD0;
	config_cdc.pinmux_pad1 = EDBG_CDC_SERCOM_PINMUX_PAD1;
	config_cdc.pinmux_pad2 = EDBG_CDC_SERCOM_PINMUX_PAD2;
	config_cdc.pinmux_pad3 = EDBG_CDC_SERCOM_PINMUX_PAD3;
	stdio_serial_init(&cdc_instance, EDBG_CDC_MODULE, &config_cdc);
	usart_enable(&cdc_instance);
}
//! [cdc_setup]

//! [can_init_setup]
static void configure_can(void)
{
	uint32_t i;

	/* Set up the CAN TX/RX pins */
	struct system_pinmux_config pin_config;
	system_pinmux_get_config_defaults(&pin_config);
	pin_config.mux_position = CAN_TX_MUX_SETTING;
	system_pinmux_pin_set_config(CAN_TX_PIN, &pin_config);
	pin_config.mux_position = CAN_RX_MUX_SETTING;
	system_pinmux_pin_set_config(CAN_RX_PIN, &pin_config);
	
	/* Initialize the module. */
	struct can_config config_can;
	can_get_config_defaults(&config_can);
	can_init(&can0_instance, CAN_MODULE, &config_can);
	
	can_enable_fd_mode(&can0_instance);
	can_start(&can0_instance);
	
	/* Enable interrupts for this CAN module */
	system_interrupt_enable(SYSTEM_INTERRUPT_MODULE_CAN0);
	system_interrupt_enable(SYSTEM_INTERRUPT_MODULE_CAN1);
	can_enable_interrupt(&can0_instance, CAN_PROTOCOL_ERROR_ARBITRATION
	| CAN_PROTOCOL_ERROR_DATA);
}
//! [can_init_setup]

void CAN0_Handler(void) {
	volatile uint32_t rx_buffer_index;
	volatile uint32_t status = can_read_interrupt_status(&can0_instance);

	if ((status & CAN_PROTOCOL_ERROR_ARBITRATION)
	|| (status & CAN_PROTOCOL_ERROR_DATA)) {
		can_clear_interrupt_status(&can0_instance, CAN_PROTOCOL_ERROR_ARBITRATION
		| CAN_PROTOCOL_ERROR_DATA);
		printf("Protocol error, please double check the clock in two boards. \r\n\r\n");
	}

	else if (status & CAN_RX_BUFFER_NEW_MESSAGE) {
		can_clear_interrupt_status(&can0_instance,CAN_RX_BUFFER_NEW_MESSAGE);
		for (int i = 0; i < NETWORK_MAX_BUFFER_ELEMENTS; i++) {
			if (can_rx_get_buffer_status(&can0_instance, i)) {
				rx_buffer_index = i;
				can_rx_clear_buffer_status(&can0_instance, i);
				int temp_Buff = CAN0_rx_element_buff[rx_buffer_index].last_write;
				can_get_rx_buffer_element(&can0_instance, &(CAN0_rx_element_buff[rx_buffer_index].buffers[temp_Buff]),
				rx_buffer_index);
				CAN0_rx_element_buff[rx_buffer_index].last_write = (temp_Buff + 1) % NETWORK_MAX_BUFFER_ELEMENTS;
			}
		}
	}
}


#define TIMEVAL (ul_tickcount+(14400000UL-1-SysTick->VAL)/48000UL)

#include <string.h>

struct node parent_broadcast_info;
struct node parent_table[10] = { 0 };
int parent_max_nodes = 10;
int parent_num_nodes = 1;
struct node* parent_info = &parent_table[0];
struct node* my_parent_info = NULL;

enum node_type my_node_type = SYSTEM_NODE_TYPE;
char* node_my_name;

void hid_get(struct hardware_id* hid) {
	hid->data = HARDWARE_ID_VAL;
}

struct node* client_discover(struct node* node_table, int max_nodes, int* num_nodes, struct node* parent_info, struct node* broadcast_info) {
	struct node_msg_discover_init disc_init;
	debug_print("Waiting for init\n");
	node_msg_wait(broadcast_info, &disc_init);
	memmove((uint8_t*)&parent_info->hid, (uint8_t*)&disc_init.hid, sizeof(struct hardware_id));

	struct hardware_id my_hid;
	hid_get(&my_hid);

	struct node_msg_discover_response disc_resp;
	memmove((uint8_t*)&disc_resp.hid, (uint8_t*)&my_hid, sizeof(struct hardware_id));
	disc_resp.capability = my_node_type;
	node_msg_send(broadcast_info, parent_info, NODE_CMD_DISC_RESP, &disc_resp);

	struct node_msg_discover_assign disc_asgn;
	debug_print("Waiting for assignment\n");
	do {
		node_msg_wait(broadcast_info, &disc_asgn);
	} while (memcmp(&disc_asgn.hid, &my_hid, sizeof(struct hardware_id)) != 0);
	debug_print("Assigned addr value %d\n", disc_asgn.addr);

	// Fill in this node's information in the table
	struct node* my_info;
	my_info = &node_table[disc_asgn.addr];
	my_info->addr.hops[0] = disc_asgn.addr;
	my_info->addr.len = 1;
	memmove((uint8_t*)&my_info->hid, (uint8_t*)&my_hid, sizeof(struct hardware_id));
	my_info->haddr.CAN_addr = disc_asgn.addr;
	my_info->type = my_node_type;
	my_info->network = parent_info->network;
	return my_info;
}

void server_discover(struct node* node_table, int max_nodes, int* num_nodes, struct node* my_info, struct node* broadcast_info) {
	my_info->addr.hops[0] = 0;

	struct node_msg_discover_init disc_init;
	memmove((uint8_t*)&disc_init.hid, (uint8_t*)&my_info->hid, sizeof(struct hardware_id));
	node_msg_send(my_info, broadcast_info, NODE_CMD_DISC_INIT, &disc_init);

	debug_print("Waiting for responses\n");

	// collect all responses for maybe a 125-1000 msec
	// immediately assign each responding node an address
	for (; *num_nodes < max_nodes; (*num_nodes)++) {
		// initialize the node struct as broadcast
		node_make_broadcast(my_info->network, &node_table[*num_nodes]);

		struct node_msg_discover_response disc_resp;
		node_msg_wait(&node_table[*num_nodes], &disc_resp);
		debug_print("Discover response cmd: %d, ret: %d, hid %4x\n", disc_resp.header.cmd, disc_resp.header.ret, *((uint32_t*)&disc_resp.hid));
		// after this, the physical address of the node is filled in, but an ID has not been assigned

		struct node_msg_discover_assign disc_asgn;
		memmove((uint8_t*)&disc_asgn.hid, (uint8_t*)&disc_resp.hid, sizeof(struct hardware_id));
		disc_asgn.addr = *num_nodes;
		node_msg_send(my_info, &node_table[*num_nodes], NODE_CMD_DISC_ASGN, &disc_asgn);
		debug_print("Assigning address %d to hid %4x\n", disc_asgn.addr, *((uint32_t*)&disc_asgn.hid));
		node_table[*num_nodes].addr.hops[0] = *num_nodes;
		node_table[*num_nodes].addr.len = 1;
		memmove((uint8_t*)&node_table[*num_nodes].hid, (uint8_t*)&disc_resp.hid, sizeof(struct hardware_id));
		node_table[*num_nodes].type = disc_resp.capability;
		node_table[*num_nodes].haddr.CAN_addr = *num_nodes;
	}
}

int main(void)
{
	//uint8_t key;

//! [setup_init]
system_init();
configure_usart_cdc();

//! [setup_init]
//TODO: Unsure if SysTick works right now. Give me some time.
/*SysTick->CTRL = 0;					// Disable SysTick
SysTick->LOAD = 14400000UL-1;			// Set reload register for 1mS interrupts
NVIC_SetPriority(SysTick_IRQn, 3);	// Set interrupt priority to least urgency
SysTick->VAL = 0;					// Reset the SysTick counter value
SysTick->CTRL = 0x00000007;			// Enable SysTick, Enable SysTick Exceptions, Use CPU Clock
NVIC_EnableIRQ(SysTick_IRQn);		// Enable SysTick Interrupt*/

//configure_rtc_count();
//rtc_count_set_period(&rtc_instance, 2000);
//! [main_setup]
int last_num = 0;
//! [configure_can]
configure_can();
//! [configure_can]
struct port_config config_port_pin;
port_get_config_defaults(&config_port_pin);
config_port_pin.direction = PORT_PIN_DIR_OUTPUT;
port_pin_set_config(LED_0_PIN, &config_port_pin);
port_pin_set_output_level(LED_0_PIN, 1);
printf("Starting\r\n");
node_my_name = myname;
InitKWIDKeyData();
if (SYSTEM_NODE_TYPE == NODE_TYPE_MOTOR)
	initMotor();
else if (SYSTEM_NODE_TYPE == NODE_TYPE_FAN)
	initFan();
else if (SYSTEM_NODE_TYPE == NODE_TYPE_TSENS)
	initTSENS();

struct network parent_net;
parent_net.can_instance = &can0_instance;
parent_net.buff_num = CAN0_COMM_BUFF_INDEX;
parent_net.broadcast_buff_num = CAN0_BROADCAST_INDEX;
parent_net.rx_element_buff = CAN0_rx_element_buff;
node_make_broadcast(&parent_net,&parent_broadcast_info);
node_make_parent(&parent_net,parent_info);
network_start_listening(&parent_net,&parent_broadcast_info.haddr);
my_parent_info = client_discover(parent_table, parent_max_nodes, &parent_num_nodes, parent_info, &parent_broadcast_info);
for (int i = 0; i < parent_num_nodes; i++) {
	parent_table[i].encryption_data = &selfData.ASCON_data; // or whatever
}
parent_broadcast_info.encryption_data = &selfData.ASCON_data;
my_parent_info->encryption_data = &selfData.ASCON_data;
network_start_listening(&parent_net,&my_parent_info->haddr);
authToParent(my_parent_info,parent_info,&parent_broadcast_info);
port_pin_set_output_level(LED_0_PIN, 0);
//my_parent_info->haddr.type = parent_net.type;

int test_val = 0;
while (1) {
	struct node_msg_generic msg;
	if (node_msg_check(&parent_broadcast_info, &msg)) {
		if (msg.header.TTL == 0) {
			switch (msg.header.cmd) {
				case NODE_CMD_READ: {
					struct node_msg_data_addr* msg_read = (void*)&msg;
					debug_print("Received read request from %d for addr %x\n", msg_read->header.ret, msg_read->addr);
					uint32_t result;
					switch(msg_read->addr) {
						case CB_VAL_STATUS:
							result = (cap_callback_ref.status_cb_get)();
							break;
						case CB_VAL_STATUS_ALT:
							result = (cap_callback_ref.status_cb_get_alt)();
							break;
						case CB_TEST:
							result = test_val;
				}
				struct node source;
				node_make_source_from_msg(my_parent_info->network, &source, parent_table, parent_num_nodes, msg_read);

				struct node_msg_data_addr_value msg_read_resp;
				msg_read_resp.addr = msg_read->addr;
				msg_read_resp.value = result;
				debug_print("Sending response to %d, addr %x, data %x\n", msg_read->header.ret, msg_read_resp.addr, msg_read_resp.value);
				node_msg_send(my_parent_info, &source, NODE_CMD_READ_RESP, &msg_read_resp);
				break;
			}
				
				case NODE_CMD_WRITE: {
					struct node_msg_data_addr_value* msg_write = (void*)&msg;
					debug_print("Received write request from %d for addr %x, data %x\n", msg_write->header.ret, msg_write->addr, msg_write->value);
					uint32_t result;
					switch(msg_write->addr) {
						case CB_VAL_STATUS:
						(cap_callback_ref.write_cb_set)(msg_write->value);
						break;
						case CB_VAL_STATUS_ALT:
						(cap_callback_ref.write_cb_toggle)();
						break;
						case CB_TEST:
						result = test_val;
					}
					struct node source;
					node_make_source_from_msg(my_parent_info->network, &source, parent_table, parent_num_nodes, msg_write);

					struct node_msg_data_addr msg_write_resp;
					msg_write_resp.addr = msg_write->addr;
					debug_print("Sending response to %d, addr %x\n", msg_write->header.ret, msg_write_resp.addr);
					node_msg_send(my_parent_info, &source, NODE_CMD_READ_RESP, &msg_write_resp);
					break;
				}
			}
		}
	}
}

//! [main_loop]

//! [main_setup]
}