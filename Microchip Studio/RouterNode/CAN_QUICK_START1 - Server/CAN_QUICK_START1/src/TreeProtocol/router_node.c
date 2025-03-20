#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "network.h"
#include "node.h"


struct node parent_broadcast_info;
struct node parent_table[10] = { 0 };
int parent_max_nodes = 3;
int parent_num_nodes = 1;
struct node* parent_info = &parent_table[0];
struct node* my_parent_info = NULL;
struct node child_broadcast_info;
struct node children_table[10] = { 0 };
int child_max_nodes = 3;
int child_num_nodes = 1;
struct node* my_child_info = &children_table[0];

enum node_type my_node_type = NODE_TYPE_ROUTER;
char* node_my_name;

int node_types[] = { 2, 3 };

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

void server_discover(struct node* node_table, int max_nodes, int* num_nodes, struct node* my_info, struct node* broadcast_info) {
	for (int i = *num_nodes; i < max_nodes; i++) {
		int child_fd = fork();
		if (child_fd == 0) {
			close(my_info->network->udp_info.fd);
			close(my_parent_info->network->udp_info.fd);
			char name_buf[30];
			sprintf(name_buf, "%s_%d", node_type_to_typename(node_types[i - 1]), my_info->addr.hops[0] + i);
			char port_buf[10];
			sprintf(port_buf, "%d", my_info->network->udp_info.base_port);
			char id_buf[10];
			sprintf(id_buf, "%d", my_info->addr.hops[0] + i);
			char target_buf[30];
			sprintf(target_buf, "./%s_node.exe", node_type_to_typename(node_types[i - 1]));
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
	if (argc != 4) {
		printf("Usage: %s name port id\n", argv[0]);
		return 0;
	}
	node_my_name = argv[1];
	int network_base_port = atoi(argv[2]);
	int my_id = atoi(argv[3]);
	int my_child_id = my_id * 10;
	int parent_id = my_id / 10 * 10;
	srand(my_id);
	debug_print("ID in parent %d, ID in self %d, port %d, parent port %d\n", my_id, my_child_id, network_base_port + my_id, network_base_port + parent_id);

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


	// Initialize underlying network
	struct network child_net;
	udp_init(my_child_id, network_base_port, &child_net.udp_info);
	debug_print("Finished udp_init on child net\n");

	my_child_info->network = &child_net;
	my_child_info->haddr.type = NETWORK_TYPE_UDP;
	my_child_info->addr.hops[0] = my_child_id;
	network_init(NETWORK_TYPE_UDP, &child_net);

	node_make_broadcast(&child_net, &child_broadcast_info);
	server_discover(children_table, child_max_nodes, &child_num_nodes, my_child_info, &child_broadcast_info);
	debug_print("Discover complete.\n");

	/*struct node_msg_data_addr msg_read;
	msg_read.addr = 0;
	debug_print("Sending request as %d with cmd %d for addr %x\n", my_child_info->addr.hops[0], NODE_CMD_READ, msg_read.addr);
	node_msg_send(my_child_info, &children_table[1], NODE_CMD_READ, &msg_read);

	struct node_msg_data_addr_value msg_read_resp;
	debug_print("Waiting for read response\n");
	//do {
		//debug_print("Received cmd %d, addr %x, value %x\n", msg_read_resp.header.cmd, msg_read_resp.addr, msg_read_resp.value);
		node_msg_wait(my_child_info, &msg_read_resp);
	//} while (msg_read_resp.header.cmd != NODE_CMD_READ_RESP);
	debug_print("Received response cmd %d, addr %x, value %x\n", msg_read_resp.header.cmd, msg_read_resp.addr, msg_read_resp.value);*/

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
						struct node source;
						node_make_source_from_msg(my_parent_info->network, &source, parent_table, parent_num_nodes, &msg_cap);
						memmove(&msg_naddr.addr, &target->addr, sizeof(struct node_addr));
						debug_print("Responding to request for network address of capability %d with address %x:%x l %d\n", msg_cap->cap, msg_naddr.addr.hops[0], msg_naddr.addr.hops[1], msg_naddr.addr.len);
						node_msg_send(my_child_info, &source, NODE_CMD_NAD, &msg_naddr);
					}
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
						struct node source;
						node_make_source_from_msg(my_parent_info->network, &source, parent_table, parent_num_nodes, &msg_cap);
						msg_cap.cap = target->type;
						debug_print("Responding to request for capability of address %d with capability %d\n", msg_naddr->addr.hops[0], msg_cap.cap);
						node_msg_send(my_child_info, &source, NODE_CMD_CAP, &msg_cap);
					}
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
						struct node source;
						node_make_source_from_msg(my_parent_info->network, &source, parent_table, parent_num_nodes, msg_cap);
						memmove(&msg_naddr.addr, &target->addr, sizeof(struct node_addr));
						debug_print("Responding to request for network address of capability %d with address %x:%x l %d\n", msg_cap->cap, msg_naddr.addr.hops[0], msg_naddr.addr.hops[1], msg_naddr.addr.len);
						node_msg_send(my_parent_info, &source, NODE_CMD_NAD, &msg_naddr);
					}
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
						struct node source;
						node_make_source_from_msg(my_parent_info->network, &source, parent_table, parent_num_nodes, msg_naddr);
						msg_cap.cap = target->type;
						debug_print("Responding to request for capability of address %d with capability %d\n", msg_naddr->addr.hops[0], msg_cap.cap);
						node_msg_send(my_parent_info, &source, NODE_CMD_CAP, &msg_cap);
					}
					break;
				}
				}
			}
		}
	}

	debug_print("Testing complete.\n");
	exit(0);
}