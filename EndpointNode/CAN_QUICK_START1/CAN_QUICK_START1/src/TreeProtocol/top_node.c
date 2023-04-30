#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>
#include <netinet/in.h>
#include "utils.h"
#include "network.h"
#include "node.h"

//struct node parent_info;
struct node child_broadcast_info;
struct node children_table[10] = { 0 };
int child_max_nodes = 3;
int child_num_nodes = 1;
struct node* my_child_info = &children_table[0];

char* node_my_name = "top";

int node_types[] = { 1, 1 };

void server_discover(struct node* node_table, int max_nodes, int* num_nodes, struct node* my_info, struct node* broadcast_info) {
	for (int i = *num_nodes; i < max_nodes; i++) {
		int child_fd = fork();
		if (child_fd == 0) {
			close(my_info->network->udp_info.fd);
			char name_buf[30];
			sprintf(name_buf, "%s_%d", node_type_to_typename(node_types[i-1]), my_info->addr.hops[0] + i);
			char port_buf[10];
			sprintf(port_buf, "%d", my_info->network->udp_info.base_port);
			char id_buf[10];
			sprintf(id_buf, "%d", my_info->addr.hops[0] + i);
			char target_buf[30];
			sprintf(target_buf, "./%s_node.exe", node_type_to_typename(node_types[i-1]));
			char* args[] = { target_buf,name_buf,port_buf,id_buf,NULL };
			debug_print("Spawning %s with port %s and id %s\n", name_buf, port_buf, id_buf);
			execvp(target_buf, args);
		}
	}
	my_info->addr.hops[0] = 0;
	usleep(125000);

	struct node_msg_discover_init disc_init;
	node_msg_send(my_info, broadcast_info, NODE_CMD_DISC_INIT, &disc_init);

	debug_print("Waiting for responses\n");

	// collect all responses for maybe a 125-1000 msec
	// immediately assign each responding node an address
	for (; *num_nodes < max_nodes; (*num_nodes)++) {
		// initialize the node struct as broadcast
		node_make_broadcast(my_info->network, &node_table[*num_nodes]);

		struct node_msg_discover_response disc_resp;
		node_msg_wait_any_assign(&node_table[*num_nodes], &disc_resp);
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
	}
}

int main(int argc, char* argv[]) {
	if (argc != 2) {
		printf("Usage: %s port\n", argv[0]);
		return 0;
	}
	char input_buffer[64];
	int network_base_port = atoi(argv[1]);
	int my_id = 0;
	int my_child_id = my_id * 10;
	debug_print("ID %d, port %d\n", my_id, network_base_port + my_id);

	struct network child_net;
	udp_init(my_id, network_base_port, &child_net.udp_info);
	debug_print("Finished udp_init\n");

	my_child_info->network = &child_net;
	my_child_info->addr.hops[0] = my_child_id;
	network_init(NETWORK_TYPE_UDP, &child_net);

	node_make_broadcast(&child_net, &child_broadcast_info);
	server_discover(children_table, child_max_nodes, &child_num_nodes, my_child_info, &child_broadcast_info);
	debug_print("Discover complete.\n");
	
	/*struct node remote;

	memmove(&remote, &children_table[1], sizeof(struct node));
	remote.addr.hops[0] = 1;
	remote.addr.hops[1] = 2;
	remote.addr.len = 2;*/

	struct node_msg_cap msg_cap;
	msg_cap.cap = NODE_TYPE_PRINTHEAD;
	debug_print("Sending request as %d with cmd %d for addr of cap %d\n", my_child_info->addr.hops[0], NODE_CMD_CAP_GNAD, msg_cap.cap);
	node_msg_send(my_child_info, &children_table[1], NODE_CMD_CAP_GNAD, &msg_cap);

	struct node_msg_nodeaddr msg_naddr;
	debug_print("Waiting for naddr response\n");
	//do {
		//debug_print("Received cmd %d, addr %x, value %x\n", msg_read_resp.header.cmd, msg_read_resp.addr, msg_read_resp.value);
	node_msg_wait(my_child_info, &msg_naddr);
	//} while (msg_read_resp.header.cmd != NODE_CMD_READ_RESP);
	debug_print("Received response cmd %d, addr %x:%x l:%d\n", msg_naddr.header.cmd, msg_naddr.addr.hops[0], msg_naddr.addr.hops[1], msg_naddr.addr.len);

	struct node remote;
	node_make_remote_from_router(my_child_info->network, &children_table[1], &remote, &msg_naddr.addr);

	struct node_msg_data_addr msg_read;
	msg_read.addr = 0;
	debug_print("Sending request as %d with cmd %d for addr %x\n", my_child_info->addr.hops[0], NODE_CMD_READ, msg_read.addr);
	node_msg_send(my_child_info, &remote, NODE_CMD_READ, &msg_read);

	struct node_msg_data_addr_value msg_read_resp;
	debug_print("Waiting for read response\n");
	//do {
		//debug_print("Received cmd %d, addr %x, value %x\n", msg_read_resp.header.cmd, msg_read_resp.addr, msg_read_resp.value);
	node_msg_wait(my_child_info, &msg_read_resp);
	//} while (msg_read_resp.header.cmd != NODE_CMD_READ_RESP);
	debug_print("Received response cmd %d, addr %x, value %x\n", msg_read_resp.header.cmd, msg_read_resp.addr, msg_read_resp.value);

	/*do {
		getline(input_buffer, 64, stdin);

	} while (strcmp(buffer, "exit") != 0);*/

	/*while (1) {
		struct node_msg_generic msg;
		if (node_msg_check(&child_broadcast_info, &msg)) {
			if (msg.header.TTL != 0) {
				node_msg_forward(children_table, child_num_nodes, my_child_info, &msg);
			}
			else {
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
						debug_print("Responding to request for network address of capability %d with address %d\n", msg_cap->cap, msg_naddr.addr.hops[0]);
						node_msg_send(my_child_info, &children_table[1], NODE_CMD_NAD, &msg_naddr);
					}
					break;
				}
				/*case NODE_CMD_HID_GNAD: {
					struct node_msg_hid* msg_hid = (void*)&msg;
					struct node_msg_nodeaddr msg_naddr;
					struct node* target = NULL;
					for (int i = 0; i < child_num_nodes; i++) {
						if (memcmp(&msg_naddr.hid, &children_table[i].hid), sizeof(struct node_addr)) {
							target = &children_table[i];
							break;
						}
					}
					if (target != NULL) {
						memmove(&msg_naddr.addr, &target->addr, sizeof(struct node_addr));
						debug_print("Responding to request for network address of HID %x with address %x\n", *((uint32_t*)&msg_hid->hid), msg_naddr.addr);
						node_msg_send(my_child_info, &children_table[1], NODE_CMD_NAD, &msg_naddr);
					}
					break;
				}*|/
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
						debug_print("Responding to request for capability of address %d with capability %d\n", msg_naddr->addr.hops[0], msg_cap.cap);
						node_msg_send(my_child_info, &children_table[1], NODE_CMD_CAP, &msg_cap);
					}
					break;
				}
				}
			}
		}
	}*/

	debug_print("Testing complete.\n");
	while (1);
	exit(0);
}