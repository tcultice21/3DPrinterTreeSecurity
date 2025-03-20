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


struct node parent_broadcast_info;
struct node parent_table[10] = { 0 };
int parent_max_nodes = 10;
int parent_num_nodes = 1;
struct node* parent_info = &parent_table[0];
struct node* my_parent_info = NULL;

enum node_type my_node_type = NODE_TYPE_SERVO;
char* node_my_name;

struct node* client_discover(struct node* node_table, int max_nodes, int* num_nodes, struct node* parent_info, struct node* broadcast_info) {
	struct node_msg_discover_init disc_init;
	debug_print("Waiting for init\n");
	node_msg_wait(broadcast_info, &disc_init);

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
	// haddr?
	my_info->type = my_node_type;
	my_info->network = parent_info->network;
	return my_info;
}

int main(int argc, char* argv[]) {
	if (argc != 4) {
		printf("Usage: %s name port id\n", argv[0]);
		return 0;
	}
	node_my_name = argv[1];
	int network_base_port = atoi(argv[2]);
	int my_id = atoi(argv[3]);
	int parent_id = my_id / 10 * 10;
	srand(my_id);
	debug_print("ID %d, port %d, parent port %d\n", my_id, network_base_port + my_id, network_base_port + parent_id);

	// Initialize underlying network
	struct network parent_net;
	udp_init(my_id, network_base_port, &parent_net.udp_info);
	//debug_print("Finished udp_init on parent net\n");

	// Initialize the parent network
	network_init(NETWORK_TYPE_UDP, &parent_net);

	// initialize parent_info and parent broadcast
	node_make_parent(&parent_net, parent_info);
	node_make_broadcast(&parent_net, &parent_broadcast_info);
	debug_print("Parent port %d\n", parent_info->addr.hops[0] + parent_net.udp_info.base_port);
	my_parent_info = client_discover(parent_table, parent_max_nodes, &parent_num_nodes, parent_info, &parent_broadcast_info);
	my_parent_info->haddr.type = parent_net.type;
	debug_print("Discover complete.\n");

	while (1) {
		struct node_msg_generic msg;
		if (node_msg_check(&parent_broadcast_info, &msg)) {
			if (msg.header.TTL == 0) {
				switch (msg.header.cmd) {
				case NODE_CMD_READ: {
					struct node_msg_data_addr* msg_read = (void*)&msg;
					debug_print("Received read request from %d for addr %x\n", msg_read->header.ret, msg_read->addr);
					uint32_t result;
					switch (msg_read->addr) {
					case 0:
						result = my_node_type;
						break;
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
				}
			}
		}
	}

	debug_print("Testing complete.\n");
	exit(0);
}