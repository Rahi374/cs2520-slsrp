#ifndef _ROUTER_DEFS_
#define _ROUTER_DEFS_

#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>

#include "list.h"

#define MAX_UNACKED_ALIVE_MESSAGES 10

extern pthread_mutex_t mutex_neighbours_list;
extern struct neighbour *neighbours_list;
extern struct table *hm_alive;

extern struct in_addr cur_router_id;
extern int cur_router_port;

enum packet_type {
	NAK,
	ACK,
	NEIGHBOR_REQ,
	NEIGHBOR_REQ_RESP,
	LSA,
	ALIVE,
	ALIVE_RESP,
	LINK_COST,
	LINK_DOWN,
	UI_CONTROL_ADD_NEIGHBOUR,
	UI_CONTROL_KILL_NEIGHBOUR,
	UI_CONTROL_GIMME_ROUTING_TABLE,
	TEST_PACKET,
};

// depending on the packet type, the length might simply contain the data
// NEIGHBOR_REQ - no data
// NEIGHBOR_REQ_RESP - 0 for negative, 1 for affirmative
struct packet_header {
	struct in_addr source_addr;
	unsigned int source_port;
	struct in_addr destination_addr;
	unsigned int destination_port;
	enum packet_type packet_type;
	int length;
	unsigned int checksum_header;
	unsigned int checksum_data;
};

struct packet {
	struct packet_header *header;
	void *data;
	int sock;
};

struct add_neighbour_command {
	struct in_addr neighbour_addr;
	unsigned int neighbour_port;
};

struct neighbour {
	unsigned int id;
	struct list_head list;
};

struct alive_control_struct {
	pthread_mutex_t mutex_alive_control_struct;
	pid_t pid_of_control_thread;
	int num_unacked_messages;
	struct in_addr n_addr;
	int n_port;
};

#endif // _ROUTER_DEFS_
