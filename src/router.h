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

enum packet_type {
	NAK,
	ACK,
	UI_CONTROL,
	NEIGHBOR_REQ,
	NEIGHBOR_REQ_RESP,
	LSA,
	ALIVE,
	ALIVE_RESP,
	LINK_COST,
	LINK_DOWN,
	TEST_PACKET,
};

// depending on the packet type, the length might simply contain the data
// NEIGHBOR_REQ - no data
// NEIGHBOR_REQ_RESP - 0 for negative, 1 for affirmative
struct packet_header {
	unsigned int source_id;//TODO get rid of
	struct in_addr source_addr;
	unsigned int source_port;
	unsigned int destination_id;//TODO get rid of
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
