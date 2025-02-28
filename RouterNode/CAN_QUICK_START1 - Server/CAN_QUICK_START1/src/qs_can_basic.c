/**
 * \file
 *
 * \brief SAM CAN basic Quick Start
 *
 * Copyright (c) 2015-2018 Microchip Technology Inc. and its subsidiaries.
 *
 * \asf_license_start
 *
 * \page License
 *
 * Subject to your compliance with these terms, you may use Microchip
 * software and any derivatives exclusively with Microchip products.
 * It is your responsibility to comply with third party license terms applicable
 * to your use of third party software (including open source software) that
 * may accompany Microchip software.
 *
 * THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS". NO WARRANTIES,
 * WHETHER EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE,
 * INCLUDING ANY IMPLIED WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY,
 * AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT WILL MICROCHIP BE
 * LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE, INCIDENTAL OR CONSEQUENTIAL
 * LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND WHATSOEVER RELATED TO THE
 * SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP HAS BEEN ADVISED OF THE
 * POSSIBILITY OR THE DAMAGES ARE FORESEEABLE.  TO THE FULLEST EXTENT
 * ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL CLAIMS IN ANY WAY
 * RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT OF FEES, IF ANY,
 * THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS SOFTWARE.
 *
 * \asf_license_stop
 *
 */
/*
 * Support and FAQ: visit <a href="https://www.microchip.com/support/">Microchip Support</a>
 */
#include <asf.h>
#include <string.h>
#include <conf_can.h>
#include "EncLib.h"

//! [module_var]

//! [module_inst]
static struct usart_module cdc_instance;
uint32_t ul_tickcount = 0;
//! [module_inst]

struct node parent_broadcast_info;
struct node parent_table[10] = { 0 };
int parent_max_nodes = EXPECTED_NUM_SIBLINGS;
int parent_num_nodes = 1;
struct node* parent_info = &parent_table[0];
struct node* my_parent_info = NULL;
struct node child_broadcast_info;
struct node children_table[10] = { 0 };
int child_max_nodes = EXPECTED_NUM_CHILDREN;
int child_num_nodes = 1;
struct node* my_child_info = &children_table[0];

enum node_type my_node_type = NODE_TYPE_ROUTER;
char* node_my_name;
	
//! [module_var]

//! [setup]

int hexchar_to_int(char input) {
	if (input >= '0' && input <= '9') {
		return input - '0';
	}
	else if (input >= 'A' && input <= 'F') {
		return input - 'A' + 10;
	}
	else if (input >= 'a' && input <= 'f') {
		return input - 'a' + 10;
	}
}

char get_char();

char getchar_wrapper() {
	char to_return;
	//uint16_t temp;
	do {
		to_return = get_char();
		//while(usart_read_wait(&cdc_instance,&temp) != STATUS_OK);
		//to_return = (char)temp;
		//debug_print("Received character %d\n", to_return);
	} while (to_return == '\r');
	return to_return;
}

struct supervisor_command {
	uint8_t endpoint;
	uint8_t cmd; // 0 for read, 1 for write
	union {
		struct {
			uint8_t read_addr;
		};
		struct {
			uint8_t write_addr;
			uint8_t write_data;
		};
	};
};

struct supervisor_response {
	uint8_t endpoint;
	uint8_t cmd; // 0 for read response, 1 for write response;
	union {
		struct {
			uint8_t read_addr;
			uint8_t read_data;
		};
		struct {
			uint8_t write_addr;
		};
	};
};

#define MAX_RX_BUFFER_LENGTH   64
struct rx_buffer {
	volatile uint8_t last_write;
	volatile uint8_t last_read;
	volatile uint8_t buffers[MAX_RX_BUFFER_LENGTH];
};
struct rx_buffer rx_buffer = {0};

char get_char() {
	while (rx_buffer.last_write == rx_buffer.last_read);
	register int last_element = rx_buffer.last_read;
	
	rx_buffer.last_read = (rx_buffer.last_read + 1) % MAX_RX_BUFFER_LENGTH;
	//printf("Escaped...\r\n");
	return rx_buffer.buffers[last_element];
}

//! [callback_funcs]
void usart_read_callback(uint8_t instance)
{
	struct usart_module *usart_module
	= (struct usart_module *)_sercom_instances[instance];
	//usart_write_buffer_job(&cdc_instance,
	//(uint8_t *)rx_buffer, MAX_RX_BUFFER_LENGTH);
	uint16_t temp = (usart_module->hw->USART.DATA.reg & SERCOM_USART_DATA_MASK);
	//printf("Called BACK %i\r\n",temp);
	rx_buffer.buffers[rx_buffer.last_write] = (char)temp;
	rx_buffer.last_write = (rx_buffer.last_write + 1) % MAX_RX_BUFFER_LENGTH;
}

//! [setup]
void configure_usart(void)
{
	//! [setup_config]
	struct usart_config config_usart;
	//! [setup_config]
	//! [setup_config_defaults]
	usart_get_config_defaults(&config_usart);
	//! [setup_config_defaults]
	config_usart.baudrate    = 115200;
	config_usart.mux_setting = EDBG_CDC_SERCOM_MUX_SETTING;
	config_usart.pinmux_pad0 = EDBG_CDC_SERCOM_PINMUX_PAD0;
	config_usart.pinmux_pad1 = EDBG_CDC_SERCOM_PINMUX_PAD1;
	config_usart.pinmux_pad2 = EDBG_CDC_SERCOM_PINMUX_PAD2;
	config_usart.pinmux_pad3 = EDBG_CDC_SERCOM_PINMUX_PAD3;
	//! [setup_change_config]

	//! [setup_set_config]
	/*while (usart_init(&cdc_instance,
	EDBG_CDC_MODULE, &config_usart) != STATUS_OK) {
	}*/
	//! [setup_set_config]
	stdio_serial_init(&cdc_instance, EDBG_CDC_MODULE, &config_usart);
	usart_enable(&cdc_instance);
}

void configure_usart_callbacks(void)
{
	//! [setup_register_callbacks]
	usart_register_callback(&cdc_instance,
	usart_read_callback, USART_CALLBACK_BUFFER_RECEIVED);
	//! [setup_register_callbacks]

	//! [setup_enable_callbacks]
	usart_enable_callback(&cdc_instance, USART_CALLBACK_BUFFER_RECEIVED);
	system_interrupt_enable_global();
	//! [setup_enable_callbacks]
	uint8_t instance_index = _sercom_get_sercom_inst_index(cdc_instance.hw);
	_sercom_set_handler(instance_index, usart_read_callback);
	_sercom_instances[instance_index] = &cdc_instance;
	cdc_instance.hw->USART.INTENSET.reg = SERCOM_USART_INTFLAG_RXC;
}
//! [setup]

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
	
	// Set up the CAN1 TX/RX pin
	pin_config.mux_position = CAN_TX_MUX_SETTING_2;
	system_pinmux_pin_set_config(CAN_TX_PIN_2,&pin_config);
	pin_config.mux_position = CAN_RX_MUX_SETTING_2;
	system_pinmux_pin_set_config(CAN_RX_PIN_2,&pin_config);
	
	/* Initialize the module. */
	struct can_config config_can;
	can_get_config_defaults(&config_can);
	can_init(&can0_instance, CAN_MODULE, &config_can);
	
	can_enable_fd_mode(&can0_instance);
	can_start(&can0_instance);
	
	#if defined(SYSTEM_ROUTER_TYPE)
	can_init(&can1_instance, CAN_MODULE_2, &config_can);
	
	can_enable_fd_mode(&can1_instance);
	can_start(&can1_instance);
	#endif
	
	/* Enable interrupts for this CAN module */
	system_interrupt_enable(SYSTEM_INTERRUPT_MODULE_CAN0);
	system_interrupt_enable(SYSTEM_INTERRUPT_MODULE_CAN1);
	can_enable_interrupt(&can0_instance, CAN_PROTOCOL_ERROR_ARBITRATION
	| CAN_PROTOCOL_ERROR_DATA);
	#if defined(SYSTEM_ROUTER_TYPE)
	can_enable_interrupt(&can1_instance, CAN_PROTOCOL_ERROR_ARBITRATION
	| CAN_PROTOCOL_ERROR_DATA);
	#endif
}
//! [can_init_setup]

void CAN0_Handler(void) {
	volatile uint32_t rx_buffer_index;
	volatile uint32_t status = can_read_interrupt_status(&can0_instance);
	//printf("Status = %i, Pubkey[2][6] = %02x",status,StoredPublicKeys[2][6]);

	if ((status & CAN_PROTOCOL_ERROR_ARBITRATION)
	|| (status & CAN_PROTOCOL_ERROR_DATA)) {
		can_clear_interrupt_status(&can0_instance, CAN_PROTOCOL_ERROR_ARBITRATION
			| CAN_PROTOCOL_ERROR_DATA);
		//printf("Protocol error, please double check the clock in two boards. \r\n\r\n");
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
				
// 				if (rx_element_buff[rx_buffer_index].buffers[temp_Buff].R0.bit.XTD) {
//  					printf("\n\r Extended message received in Rx buffer. The received data is: \r\n");
//  					} else {
//  					printf("\n\r Standard message received in Rx buffer %d, section %d. The received data is: \r\n",rx_buffer_index,temp_Buff);
//  				}
//  				for (i = 0; i < rx_element_buff[rx_buffer_index].buffers[temp_Buff].R1.bit.DLC; i++) {
//  					printf("  %d",rx_element_buff[rx_buffer_index].buffers[temp_Buff].data[i]);
//  				}
//  				printf("\r\n\r\n");
			}
		}
	}
}
#if defined(SYSTEM_ROUTER_TYPE)
void CAN1_Handler(void) {
	volatile uint32_t rx_buffer_index;
	volatile uint32_t status = can_read_interrupt_status(&can1_instance);

	if ((status & CAN_PROTOCOL_ERROR_ARBITRATION)
	|| (status & CAN_PROTOCOL_ERROR_DATA)) {
		can_clear_interrupt_status(&can1_instance, CAN_PROTOCOL_ERROR_ARBITRATION
		| CAN_PROTOCOL_ERROR_DATA);
		//printf("Protocol error, please double check the clock in two boards. \r\n\r\n");
	}

	else if (status & CAN_RX_BUFFER_NEW_MESSAGE) {
			can_clear_interrupt_status(&can1_instance,CAN_RX_BUFFER_NEW_MESSAGE);
			for (int i = 0; i < NETWORK_MAX_BUFFER_ELEMENTS; i++) {
				if (can_rx_get_buffer_status(&can1_instance, i)) {
					rx_buffer_index = i;
					can_rx_clear_buffer_status(&can1_instance, i);
					int temp_Buff = CAN1_rx_element_buff[rx_buffer_index].last_write;
					can_get_rx_buffer_element(&can1_instance, &(CAN1_rx_element_buff[rx_buffer_index].buffers[temp_Buff]),
						rx_buffer_index);
					CAN1_rx_element_buff[rx_buffer_index].last_write = (temp_Buff + 1) % NETWORK_MAX_BUFFER_ELEMENTS;
					
//  					if (CAN1_rx_element_buff[rx_buffer_index].buffers[temp_Buff].R0.bit.XTD) {
//  						printf("\n\r Extended message received in Rx buffer. The received data is: \r\n");
//  	 				} else {
//  	 					printf("\n\r Standard message received in Rx buffer %d, section %d. The received data is: \r\n",rx_buffer_index,temp_Buff);
//  	 				}
//  	 				for (i = 0; i < CAN1_rx_element_buff[rx_buffer_index].buffers[temp_Buff].R1.bit.DLC; i++) {
// 						printf("  %d",CAN1_rx_element_buff[rx_buffer_index].buffers[temp_Buff].data[i]);
//  	 				}
//  	 				printf("\r\n\r\n");
  				}
			}
	}
}
#endif
//! [setup]
void SysTick_Handler(void){
	
	ul_tickcount += 10;
	
}

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

#define TIMEVAL (ul_tickcount+(48000000UL-1UL-SysTick->VAL)/48000UL)
#define PIN_PC05	69

int main(void)
{
	//uint8_t key;
//! [setup_init]
	system_init();
	configure_usart();
	configure_usart_callbacks();
	//configure_usart_cdc();
//! [setup_init]

//! [main_setup]
//! [configure_can]
	configure_can();
//! [configure_can]

//! [display_user_menu]
	//display_menu();
	node_my_name = myname;
	debug_print("Starting.\r\n");
	struct port_config config_port_pin;
	port_get_config_defaults(&config_port_pin);
	config_port_pin.direction = PORT_PIN_DIR_OUTPUT;
	port_pin_set_config(PIN_PC05, &config_port_pin);
	port_pin_set_output_level(PIN_PC05, 1);
//! [display_user_menu]

	/*SysTick->CTRL = 0;					// Disable SysTick
	SysTick->LOAD = 48000000UL-1;		// Set reload register for 1mS interrupts
	NVIC_SetPriority(SysTick_IRQn, 3);	// Set interrupt priority to least urgency
	SysTick->VAL = 0;					// Reset the SysTick counter value
	SysTick->CTRL = 0x00000007;			// Enable SysTick, Enable SysTick Exceptions, Use CPU Clock
	NVIC_EnableIRQ(SysTick_IRQn);		// Enable SysTick Interrupt*/
	
	debug_print("Hello\r\n");
	uint32_t SID = *((uint32_t *)(0x0080A00C));
	srand(SID);
	InitKWIDKeyData();
	//TODO: Test the AES ctr functions here?
	
#if defined(SYSTEM_ROUTER_TYPE)
	struct network parent_net;
	parent_net.can_instance = &can1_instance;
	parent_net.buff_num = CAN1_COMM_BUFF_INDEX;
	parent_net.broadcast_buff_num = CAN1_BROADCAST_INDEX;
	parent_net.rx_element_buff = CAN1_rx_element_buff;
	node_make_broadcast(&parent_net, &parent_broadcast_info);
	node_make_parent(&parent_net, parent_info);
	network_start_listening(&parent_net,&parent_broadcast_info.haddr);
	my_parent_info = client_discover(parent_table, parent_max_nodes, &parent_num_nodes, parent_info, &parent_broadcast_info);
	parent_broadcast_info.encryption_data = &selfData.ASCON_data;
	my_parent_info->encryption_data = &selfData.ASCON_data;
	for (int i = 0; i < parent_num_nodes; i++) {
		parent_table[i].encryption_data = &selfData.ASCON_data; // or whatever
	}
	network_start_listening(&parent_net,&my_parent_info->haddr);
	authToParent(my_parent_info,parent_info,&parent_broadcast_info);
#endif
	port_pin_set_output_level(PIN_PC05, 0);
	struct network child_net;
	child_net.can_instance = &can0_instance;
	child_net.buff_num = CAN0_COMM_BUFF_INDEX;
	child_net.broadcast_buff_num = (uint16_t)-1U;
	child_net.rx_element_buff = CAN0_rx_element_buff;
	node_make_parent(&child_net,my_child_info);
	hid_get(&my_child_info->hid);
	node_make_broadcast(&child_net, &child_broadcast_info);
	network_start_listening(&child_net,&my_child_info->haddr);
	server_discover(children_table, child_max_nodes, &child_num_nodes, my_child_info, &child_broadcast_info);
	for (int i = 0; i < child_num_nodes; i++) {
		children_table[i].encryption_data = &selfData.child_ASCON_data; // or whatever
	}
	child_broadcast_info.encryption_data = &selfData.child_ASCON_data;
	debug_print("Discover Complete.\n");
	authToChildren(children_table,child_num_nodes,my_child_info,&child_broadcast_info);
	
#if defined(SYSTEM_ROOT_ROUTER)
	// TODO: Get a collective list of all the devices
	struct node devRemotes[6];
	struct node* orderOfRouters[2];
	uint32_t NodeTypes[] = {NODE_TYPE_MOTOR,NODE_TYPE_FAN,NODE_TYPE_TSENS};
	uint32_t hidToCompare = HWIDTable[1].hw_id.data;
	if(hidToCompare == children_table[1].hid.data) {
		orderOfRouters[0] = &children_table[1];
		orderOfRouters[1] = &children_table[2];
	}
	else if (hidToCompare == children_table[2].hid.data) {
		orderOfRouters[0] = &children_table[2];
		orderOfRouters[1] = &children_table[1];
	}
	
	for(int i = 0; i < 3; i++) {
		// Now ask each node for all of their devices
		struct node_msg_cap msg_cap;
		msg_cap.cap = NodeTypes[i];
		debug_print("Sending request as %d with cmd %d for addr of cap %d\n", my_child_info->addr.hops[0], NODE_CMD_CAP_GNAD, msg_cap.cap);
		node_msg_send_generic(my_child_info, orderOfRouters[0], NODE_CMD_CAP_GNAD, &msg_cap, sizeof(struct node_msg_header) + sizeof(uint32_t));

		struct node_msg_nodeaddr msg_naddr;
		debug_print("Waiting for naddr response\n");
		//do {
		//debug_print("Received cmd %d, addr %x, value %x\n", msg_read_resp.header.cmd, msg_read_resp.addr, msg_read_resp.value);
		node_msg_wait(my_child_info, &msg_naddr);
		//} while (msg_read_resp.header.cmd != NODE_CMD_READ_RESP);
		debug_print("Received response cmd %d, addr %x:%x l:%d\n", msg_naddr.header.cmd, msg_naddr.addr.hops[0], msg_naddr.addr.hops[1], msg_naddr.addr.len);

		node_make_remote_from_router(my_child_info->network, orderOfRouters[0], &devRemotes[i], &msg_naddr.addr);
		debug_print("Entry made for %i\n",i);
	}
	for(int i = 0; i < 3; i++) {
		// Now ask each node for all of their devices
		struct node_msg_cap msg_cap;
		msg_cap.cap = NodeTypes[i];
		debug_print("Sending request as %d with cmd %d for addr of cap %d\n", my_child_info->addr.hops[0], NODE_CMD_CAP_GNAD, msg_cap.cap);
		node_msg_send_generic(my_child_info, orderOfRouters[1], NODE_CMD_CAP_GNAD, &msg_cap, sizeof(struct node_msg_header) + sizeof(uint32_t));

		struct node_msg_nodeaddr msg_naddr;
		debug_print("Waiting for naddr response\n");
		//do {
		//debug_print("Received cmd %d, addr %x, value %x\n", msg_read_resp.header.cmd, msg_read_resp.addr, msg_read_resp.value);
		node_msg_wait(my_child_info, &msg_naddr);
		//} while (msg_read_resp.header.cmd != NODE_CMD_READ_RESP);
		debug_print("Received response cmd %d, addr %x:%x l:%d\n", msg_naddr.header.cmd, msg_naddr.addr.hops[0], msg_naddr.addr.hops[1], msg_naddr.addr.len);

		node_make_remote_from_router(my_child_info->network, orderOfRouters[1], &devRemotes[i+3], &msg_naddr.addr);
		debug_print("Entry made for %i\n",i+3);
	}
	while (1) {
		char input;
		int segment_length = 0;
		int total_length = 0;
		int msg_type = 0;
		int expected_length = 0;
		int msg_failed = 0;
		uint8_t buffer[64];
		for (input = getchar_wrapper(); input != '\n' && input != ','; input = getchar_wrapper(), segment_length++) {
			int value = hexchar_to_int(input);
			//debug_print("Received character %d\n", input);
			if (value > 15 || value < 0) {
				break;
			}
			expected_length = (expected_length << 4) | hexchar_to_int(input);
		}
		//debug_print("Length Complete.\r\n");
		if (input != ',') {
			debug_print("Failed at length\n");
			msg_failed = 1;
		}
		if (!msg_failed) {
			total_length += segment_length;
			segment_length = 0;
			for (input = getchar_wrapper(); input != '\n' && input != ':'; input = getchar_wrapper(), segment_length++) {
				int value = hexchar_to_int(input);
				//debug_print("Received character %d\n", input);
				if (value > 15 || value < 0) {
					break;
				}
				msg_type = (msg_type << 4) | hexchar_to_int(input);
			}
			if (input != ':') {
				debug_print("Failed at type\n");
				msg_failed = 1;
			}
			//debug_print("Type Complete\r\n");
		}
		if (!msg_failed) {
			total_length += segment_length;
			segment_length = 0;
			if (msg_type == 0) { // raw data
				for (input = getchar_wrapper(); input != '\n'; input = getchar_wrapper(), segment_length++) {
					int value = hexchar_to_int(input);
					//debug_print("Received character %d\n", input);
					if (value > 15 || value < 0) {
						debug_print("Received invalid character %d\n", input);
						break;
					}
					if (~segment_length & 1) { // if 0th byte
						buffer[segment_length / 2] = value << 4;
					}
					else { // if 1st byte
						buffer[segment_length / 2] |= value;
					}
				}
			}
			/*else if (msg_type == 1) { // print data
				for (input = getchar(); input != '\n'; input = getchar()) {
					int value = hexchar_to_int(input);
					if (value > 15 || value < 0) {
						break;
					}
					buffer[segment_length / 2] = input << ((~segment_length & 1) * 4);
					length++;
				}
			}*/
			else {
				msg_failed = 1;
			}
			//debug_print("Data Complete\r\n");
			//for(int i = 0; i < 999999; i++);
			total_length += segment_length;
			if (total_length != expected_length) {
				debug_print("Failed because received length %d does not equal expected %d\n", total_length, expected_length);
				msg_failed = 1;
			}
		}
		while (input != '\n') {
			input = getchar();
		}
		if (!msg_failed) {
			if (msg_type == 0) { // raw data
				struct supervisor_command* command = (void*)buffer;
				if (command->cmd == 0) { //read
					if (total_length != 8) {
						debug_print("Received malformed command\n");
						continue;
					}
					struct node_msg_data_addr msg_read;
					msg_read.addr = command->read_addr;
					debug_print("Sending request as %d with cmd %d for addr %x\n", my_child_info->addr.hops[0], NODE_CMD_READ, msg_read.addr);
					node_msg_send(my_child_info, &devRemotes[command->endpoint], NODE_CMD_READ, &msg_read);

					struct node_msg_data_addr_value msg_read_resp;
					//debug_print("Waiting for read response\n");
					//do {
						//debug_print("Received cmd %d, addr %x, value %x\n", msg_read_resp.header.cmd, msg_read_resp.addr, msg_read_resp.value);
					node_msg_wait(my_child_info, &msg_read_resp);
					//} while (msg_read_resp.header.cmd != NODE_CMD_READ_RESP);
					//debug_print("Received response cmd %d, addr %x, value %x\n", msg_read_resp.header.cmd, msg_read_resp.addr, msg_read_resp.value);

					struct supervisor_response response;
					response.endpoint = command->endpoint;
					response.cmd = 0;
					response.read_addr = msg_read_resp.addr;
					response.read_data = msg_read_resp.value;
					printf("%01x,%01x:", (unsigned int)(uint8_t)10, (unsigned int)(uint8_t)0); // length = length (1 byte), type (1 byte), read_cmd (8 bytes) = 10 bytes
					for (int i = 0; i < 4; i++) {
						printf("%02x", (unsigned int)((uint8_t*)&response)[i]);
					}
					printf("\n");
				}
				else if (command->cmd == 1) { // write
					if (total_length != 10) {
						debug_print("Received malformed command\n");
						continue;
					}
					struct node_msg_data_addr_value msg_write;
					msg_write.addr = command->write_addr;
					msg_write.value = command->write_data;
					debug_print("Sending request as %d with cmd %d for addr %x, data %x\n", my_child_info->addr.hops[0], NODE_CMD_READ, msg_write.addr, msg_write.value);
					node_msg_send(my_child_info, &devRemotes[command->endpoint], NODE_CMD_WRITE, &msg_write);

					struct node_msg_data_addr msg_write_resp;
					//debug_print("Waiting for write response\n");
					//do {
						//debug_print("Received cmd %d, addr %x, value %x\n", msg_read_resp.header.cmd, msg_read_resp.addr, msg_read_resp.value);
					node_msg_wait(my_child_info, &msg_write_resp);
					//} while (msg_read_resp.header.cmd != NODE_CMD_READ_RESP);
					//debug_print("Received response cmd %d, addr %x\n", msg_write_resp.header.cmd, msg_write_resp.addr);

					struct supervisor_response response;
					response.endpoint = command->endpoint;
					response.cmd = 1;
					response.write_addr = msg_write_resp.addr;
					printf("%01x,%01x:", (unsigned int)8, (unsigned int)0); // length = length (1 byte), type (1 byte), write_cmd (6 bytes) = 8 bytes
					for (int i = 0; i < 3; i++) {
						printf("%02x", (unsigned int)((uint8_t*)&response)[i]);
					}
					printf("\n");
				}
			}
		}
		else {
			debug_print("Supervisor command interpretation failed.\n");
		}
	}
	/*
	{
		struct node_msg_data_addr msg_read;
		msg_read.addr = 0;
		debug_print("Sending request as %d with cmd %d for addr %x\n", my_child_info->addr.hops[0], NODE_CMD_READ, msg_read.addr);
		node_msg_send(my_child_info, &devRemotes[0], NODE_CMD_READ, &msg_read);

		struct node_msg_data_addr_value msg_read_resp;
		debug_print("Waiting for read response\n");
		//do {
		//debug_print("Received cmd %d, addr %x, value %x\n", msg_read_resp.header.cmd, msg_read_resp.addr, msg_read_resp.value);
		node_msg_wait(my_child_info, &msg_read_resp);
		//} while (msg_read_resp.header.cmd != NODE_CMD_READ_RESP);
		debug_print("Received response cmd %d, addr %x, value %x\n", msg_read_resp.header.cmd, msg_read_resp.addr, msg_read_resp.value);
	}
	{
		struct node_msg_data_addr msg_read;
		msg_read.addr = 0;
		debug_print("Sending request as %d with cmd %d for addr %x\n", my_child_info->addr.hops[0], NODE_CMD_READ, msg_read.addr);
		node_msg_send(my_child_info, &devRemotes[1], NODE_CMD_READ, &msg_read);

		struct node_msg_data_addr_value msg_read_resp;
		debug_print("Waiting for read response\n");
		//do {
		//debug_print("Received cmd %d, addr %x, value %x\n", msg_read_resp.header.cmd, msg_read_resp.addr, msg_read_resp.value);
		node_msg_wait(my_child_info, &msg_read_resp);
		//} while (msg_read_resp.header.cmd != NODE_CMD_READ_RESP);
		debug_print("Received response cmd %d, addr %x, value %x\n", msg_read_resp.header.cmd, msg_read_resp.addr, msg_read_resp.value);
	}
	{
		struct node_msg_data_addr msg_read;
		msg_read.addr = 0;
		debug_print("Sending request as %d with cmd %d for addr %x\n", my_child_info->addr.hops[0], NODE_CMD_READ, msg_read.addr);
		node_msg_send(my_child_info, &devRemotes[2], NODE_CMD_READ, &msg_read);

		struct node_msg_data_addr_value msg_read_resp;
		debug_print("Waiting for read response\n");
		//do {
		//debug_print("Received cmd %d, addr %x, value %x\n", msg_read_resp.header.cmd, msg_read_resp.addr, msg_read_resp.value);
		node_msg_wait(my_child_info, &msg_read_resp);
		//} while (msg_read_resp.header.cmd != NODE_CMD_READ_RESP);
		debug_print("Received response cmd %d, addr %x, value %x\n", msg_read_resp.header.cmd, msg_read_resp.addr, msg_read_resp.value);
	}
	{
		struct node_msg_data_addr_value msg_write;
		msg_write.addr = 0;
		msg_write.value = 75;
		debug_print("Sending request as %d with cmd %d for addr %x, data %x\n", my_child_info->addr.hops[0], NODE_CMD_READ, msg_write.addr, msg_write.value);
		node_msg_send(my_child_info, &devRemotes[0], NODE_CMD_WRITE, &msg_write);

		struct node_msg_data_addr msg_write_resp;
		debug_print("Waiting for write response\n");
		//do {
		//debug_print("Received cmd %d, addr %x, value %x\n", msg_read_resp.header.cmd, msg_read_resp.addr, msg_read_resp.value);
		node_msg_wait(my_child_info, &msg_write_resp);
		//} while (msg_read_resp.header.cmd != NODE_CMD_READ_RESP);
		debug_print("Received response cmd %d, addr %x\n", msg_write_resp.header.cmd, msg_write_resp.addr);

		struct node_msg_data_addr msg_read;
		msg_read.addr = 0;
		debug_print("Sending request as %d with cmd %d for addr %x\n", my_child_info->addr.hops[0], NODE_CMD_READ, msg_read.addr);
		node_msg_send(my_child_info, &devRemotes[0], NODE_CMD_READ, &msg_read);

		struct node_msg_data_addr_value msg_read_resp;
		debug_print("Waiting for read response\n");
		//do {
		//debug_print("Received cmd %d, addr %x, value %x\n", msg_read_resp.header.cmd, msg_read_resp.addr, msg_read_resp.value);
		node_msg_wait(my_child_info, &msg_read_resp);
		//} while (msg_read_resp.header.cmd != NODE_CMD_READ_RESP);
		debug_print("Received response cmd %d, addr %x, value %x\n", msg_read_resp.header.cmd, msg_read_resp.addr, msg_read_resp.value);
		

	}
	{
		struct node_msg_data_addr_value msg_write;
		msg_write.addr = 0;
		msg_write.value = 1;
		debug_print("Sending request as %d with cmd %d for addr %x, data %x\n", my_child_info->addr.hops[0], NODE_CMD_READ, msg_write.addr, msg_write.value);
		node_msg_send(my_child_info, &devRemotes[1], NODE_CMD_WRITE, &msg_write);

		struct node_msg_data_addr msg_write_resp;
		debug_print("Waiting for write response\n");
		//do {
		//debug_print("Received cmd %d, addr %x, value %x\n", msg_read_resp.header.cmd, msg_read_resp.addr, msg_read_resp.value);
		node_msg_wait(my_child_info, &msg_write_resp);
		//} while (msg_read_resp.header.cmd != NODE_CMD_READ_RESP);
		debug_print("Received response cmd %d, addr %x\n", msg_write_resp.header.cmd, msg_write_resp.addr);

		/*struct node_msg_data_addr msg_read;
		msg_read.addr = 0;
		debug_print("Sending request as %d with cmd %d for addr %x\n", my_child_info->addr.hops[0], NODE_CMD_READ, msg_read.addr);
		node_msg_send(my_child_info, &devRemotes[1], NODE_CMD_READ, &msg_read);

		struct node_msg_data_addr_value msg_read_resp;
		debug_print("Waiting for read response\n");
		//do {
		//debug_print("Received cmd %d, addr %x, value %x\n", msg_read_resp.header.cmd, msg_read_resp.addr, msg_read_resp.value);
		node_msg_wait(my_child_info, &msg_read_resp);
		//} while (msg_read_resp.header.cmd != NODE_CMD_READ_RESP);
		debug_print("Received response cmd %d, addr %x, value %x\n", msg_read_resp.header.cmd, msg_read_resp.addr, msg_read_resp.value);
		

	}
	for(volatile uint32_t i = 0; i < 16000000*6/3; i++);
	printf("HelloHelloHelloHelloHelloHelloHelloHelloHelloHelloHelloHelloHelloHelloHelloHelloHelloHelloHelloHelloHelloHelloHelloHelloHelloHelloHelloHelloHelloHelloHelloHelloHelloHello\r\n");
	{
		struct node_msg_data_addr_value msg_write;
		msg_write.addr = 0;
		msg_write.value = 0;
		debug_print("Sending request as %d with cmd %d for addr %x, data %x\n", my_child_info->addr.hops[0], NODE_CMD_READ, msg_write.addr, msg_write.value);
		node_msg_send(my_child_info, &devRemotes[0], NODE_CMD_WRITE, &msg_write);

		struct node_msg_data_addr msg_write_resp;
		debug_print("Waiting for write response\n");
		//do {
		//debug_print("Received cmd %d, addr %x, value %x\n", msg_read_resp.header.cmd, msg_read_resp.addr, msg_read_resp.value);
		node_msg_wait(my_child_info, &msg_write_resp);
		//} while (msg_read_resp.header.cmd != NODE_CMD_READ_RESP);
		debug_print("Received response cmd %d, addr %x\n", msg_write_resp.header.cmd, msg_write_resp.addr);


		struct node_msg_data_addr msg_read;
		msg_read.addr = 0;
		debug_print("Sending request as %d with cmd %d for addr %x\n", my_child_info->addr.hops[0], NODE_CMD_READ, msg_read.addr);
		node_msg_send(my_child_info, &devRemotes[0], NODE_CMD_READ, &msg_read);

		struct node_msg_data_addr_value msg_read_resp;
		debug_print("Waiting for read response\n");
		//do {
		//debug_print("Received cmd %d, addr %x, value %x\n", msg_read_resp.header.cmd, msg_read_resp.addr, msg_read_resp.value);
		node_msg_wait(my_child_info, &msg_read_resp);
		//} while (msg_read_resp.header.cmd != NODE_CMD_READ_RESP);
		debug_print("Received response cmd %d, addr %x, value %x\n", msg_read_resp.header.cmd, msg_read_resp.addr, msg_read_resp.value);
		

	}
	{
		struct node_msg_data_addr_value msg_write;
		msg_write.addr = 0;
		msg_write.value = 0;
		debug_print("Sending request as %d with cmd %d for addr %x, data %x\n", my_child_info->addr.hops[0], NODE_CMD_READ, msg_write.addr, msg_write.value);
		node_msg_send(my_child_info, &devRemotes[1], NODE_CMD_WRITE, &msg_write);

		struct node_msg_data_addr msg_write_resp;
		debug_print("Waiting for write response\n");
		//do {
		//debug_print("Received cmd %d, addr %x, value %x\n", msg_read_resp.header.cmd, msg_read_resp.addr, msg_read_resp.value);
		node_msg_wait(my_child_info, &msg_write_resp);
		//} while (msg_read_resp.header.cmd != NODE_CMD_READ_RESP);
		debug_print("Received response cmd %d, addr %x\n", msg_write_resp.header.cmd, msg_write_resp.addr);


		struct node_msg_data_addr msg_read;
		msg_read.addr = 0;
		debug_print("Sending request as %d with cmd %d for addr %x\n", my_child_info->addr.hops[0], NODE_CMD_READ, msg_read.addr);
		node_msg_send(my_child_info, &devRemotes[1], NODE_CMD_READ, &msg_read);

		struct node_msg_data_addr_value msg_read_resp;
		debug_print("Waiting for read response\n");
		//do {
		//debug_print("Received cmd %d, addr %x, value %x\n", msg_read_resp.header.cmd, msg_read_resp.addr, msg_read_resp.value);
		node_msg_wait(my_child_info, &msg_read_resp);
		//} while (msg_read_resp.header.cmd != NODE_CMD_READ_RESP);
		debug_print("Received response cmd %d, addr %x, value %x\n", msg_read_resp.header.cmd, msg_read_resp.addr, msg_read_resp.value);
	}
	{
		struct node_msg_data_addr msg_read;
		msg_read.addr = 0;
		debug_print("Sending request as %d with cmd %d for addr %x\n", my_child_info->addr.hops[0], NODE_CMD_READ, msg_read.addr);
		node_msg_send(my_child_info, &devRemotes[2], NODE_CMD_READ, &msg_read);

		struct node_msg_data_addr_value msg_read_resp;
		debug_print("Waiting for read response\n");
		//do {
		//debug_print("Received cmd %d, addr %x, value %x\n", msg_read_resp.header.cmd, msg_read_resp.addr, msg_read_resp.value);
		node_msg_wait(my_child_info, &msg_read_resp);
		//} while (msg_read_resp.header.cmd != NODE_CMD_READ_RESP);
		debug_print("Received response cmd %d, addr %x, value %x\n", msg_read_resp.header.cmd, msg_read_resp.addr, msg_read_resp.value);
	}
	*/
#elif defined(SYSTEM_ROUTER_TYPE) 
{
	while (1) {
		struct node_msg_generic msg;
		if (node_msg_check(&child_broadcast_info, &msg)) {
			if (msg.header.TTL != 0) {
				debug_print("Forwarding message up from %d\n", msg.header.ret);
				node_msg_forward(parent_table, parent_num_nodes, my_parent_info, &msg);
			}
			else {
				debug_print("Handling child message with cmd %d, addr %x\n", msg.header.cmd, *((uint16_t*)&msg.header.addr));
				switch (msg.header.cmd) {
					case NODE_CMD_CAP_GNAD: {
						struct node_msg_cap* msg_cap = (void*)&msg;
						struct node_msg_nodeaddr msg_naddr;
						struct node* target = NULL;
						for (int i = 0; i < child_num_nodes; i++) {
							if (msg_cap->cap == children_table[i].type) {
								target = &children_table[i];
								break;
							}
						}
						if (target != NULL) {
							memmove(&msg_naddr.addr, &target->addr, sizeof(struct node_addr));
						}
						else {
							msg_naddr.addr.len = 0;
						}
						struct node source;
						node_make_source_from_msg(my_child_info->network, &source, children_table, child_num_nodes, &msg_cap);
						debug_print("Responding to request for network address of capability %d with address %x:%x l %d\n", msg_cap->cap, msg_naddr.addr.hops[0], msg_naddr.addr.hops[1], msg_naddr.addr.len);
						node_msg_send(my_child_info, &source, NODE_CMD_NAD, &msg_naddr);
						break;
					}
					case NODE_CMD_HID_GNAD: {
						struct node_msg_hid* msg_hid = (void*)&msg;
						struct node_msg_nodeaddr msg_naddr;
						struct node* target = NULL;
						for (int i = 0; i < child_num_nodes; i++) {
							if (memcmp(&msg_hid->hid, &children_table[i].hid, sizeof(struct node_addr)) == 0) {
								target = &children_table[i];
								break;
							}
						}
						if (target != NULL) {
							memmove(&msg_naddr.addr, &target->addr, sizeof(struct node_addr));
						}
						else {
							msg_naddr.addr.len = 0;
						}
						struct node source;
						node_make_source_from_msg(my_child_info->network, &source, children_table, child_num_nodes, &msg_hid);
						debug_print("Responding to request for network address of HID %x with with address %x:%x l %d\n", *((uint32_t*)&msg_hid->hid), msg_naddr.addr.hops[0], msg_naddr.addr.hops[1], msg_naddr.addr.len);
						node_msg_send(my_child_info, &source, NODE_CMD_NAD, &msg_naddr);
						break;
					}
					case NODE_CMD_NAD_GCAP: {
						struct node_msg_nodeaddr* msg_naddr = (void*)&msg;
						struct node_msg_cap msg_cap;
						struct node* target = NULL;
						for (int i = 0; i < child_num_nodes; i++) {
							if (memcmp(&msg_naddr->addr, &children_table[i].addr, sizeof(struct node_addr))) {
								target = &children_table[i];
								break;
							}
						}
						if (target != NULL) {
							msg_cap.cap = target->type;
						}
						else {
							msg_cap.cap = NODE_TYPE_NONE;
						}
						struct node source;
						node_make_source_from_msg(my_child_info->network, &source, children_table, child_num_nodes, &msg_cap);
						debug_print("Responding to request for capability of address %d with capability %d\n", msg_naddr->addr.hops[0], msg_cap.cap);
						node_msg_send(my_child_info, &source, NODE_CMD_CAP, &msg_cap);
						break;
					}
				}
			}
		}
		if (node_msg_check(&parent_broadcast_info, &msg)) {
			if (msg.header.TTL != 0) {
				debug_print("Fowarding message down from %d\n", msg.header.ret);
				node_msg_forward(children_table, child_num_nodes, my_child_info, &msg);
			}
			else {
				debug_print("Handling parent message with cmd %d, addr %x\n", msg.header.cmd, *((uint16_t*)&msg.header.addr));
				switch (msg.header.cmd) {
					case NODE_CMD_CAP_GNAD: {
						struct node_msg_cap* msg_cap = (void*)&msg;
						struct node_msg_nodeaddr msg_naddr;
						struct node* target = NULL;
						for (int i = 0; i < child_num_nodes; i++) {
							if (msg_cap->cap == children_table[i].type) {
								target = &children_table[i];
								break;
							}
						}
						if (target != NULL) {
							memmove(&msg_naddr.addr, &target->addr, sizeof(struct node_addr));
						}
						else {
							msg_naddr.addr.len = 0;
						}
						struct node source;
						node_make_source_from_msg(my_parent_info->network, &source, parent_table, parent_num_nodes, msg_cap);
						debug_print("Responding to request for network address of capability %d with address %x:%x l %d\n", msg_cap->cap, msg_naddr.addr.hops[0], msg_naddr.addr.hops[1], msg_naddr.addr.len);
						node_msg_send(my_parent_info, &source, NODE_CMD_NAD, &msg_naddr);
						break;
					}
					case NODE_CMD_HID_GNAD: {
						struct node_msg_hid* msg_hid = (void*)&msg;
						struct node_msg_nodeaddr msg_naddr;
						struct node* target = NULL;
						for (int i = 0; i < child_num_nodes; i++) {
							if (memcmp(&msg_hid->hid, &children_table[i].hid, sizeof(struct node_addr)) == 0) {
								target = &children_table[i];
								break;
							}
						}
						if (target != NULL) {
							memmove(&msg_naddr.addr, &target->addr, sizeof(struct node_addr));
						}
						else {
							msg_naddr.addr.len = 0;
						}
						struct node source;
						node_make_source_from_msg(my_parent_info->network, &source, parent_table, parent_num_nodes, msg_hid);
						debug_print("Responding to request for network address of HID %x with with address %x:%x l %d\n", *((uint32_t*)&msg_hid->hid), msg_naddr.addr.hops[0], msg_naddr.addr.hops[1], msg_naddr.addr.len);
						node_msg_send(my_parent_info, &source, NODE_CMD_NAD, &msg_naddr);
						break;
					}
					case NODE_CMD_NAD_GCAP: {
						struct node_msg_nodeaddr* msg_naddr = (void*)&msg;
						struct node_msg_cap msg_cap;
						struct node* target = NULL;
						for (int i = 0; i < child_num_nodes; i++) {
							if (memcmp(&msg_naddr->addr, &children_table[i].addr, sizeof(struct node_addr)) == 0) {
								target = &children_table[i];
								break;
							}
						}
						if (target != NULL) {
							msg_cap.cap = target->type;
						}
						else {
							msg_cap.cap = NODE_TYPE_NONE;
						}
						struct node source;
						node_make_source_from_msg(my_parent_info->network, &source, parent_table, parent_num_nodes, msg_naddr);
						debug_print("Responding to request for capability of address %d with capability %d\n", msg_naddr->addr.hops[0], msg_cap.cap);
						node_msg_send(my_parent_info, &source, NODE_CMD_CAP, &msg_cap);
						break;
					}
				}
			}
		}
	}
}
#endif
}