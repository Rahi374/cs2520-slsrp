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
extern int neighbour_count;

extern pthread_mutex_t mutex_hm_alive;
extern struct table *hm_alive;
extern pthread_mutex_t mutex_hm_cost;
extern struct table *hm_cost;

extern pthread_mutex_t mutex_hm_lsa;
extern struct table *hm_lsa;
extern int lsa_count;

extern struct table *hm_rt_index;
extern struct rt_entry *rt;

extern struct in_addr cur_router_id;
extern int cur_router_port;

enum packet_type {
	NAK,
	ACK,
	NEIGHBOR_REQ,
	NEIGHBOR_REQ_RESP,
	LSA,
	LSA_ACK,
	ALIVE,
	ALIVE_RESP,
	LINK_COST,
	LINK_COST_RESP,
	LINK_DOWN,
	UI_CONTROL_ADD_NEIGHBOUR,
	UI_CONTROL_KILL_NEIGHBOUR,
	UI_CONTROL_GIMME_ROUTING_TABLE,
	TEST_PACKET,
};

// depending on the packet type, the length might simply contain the data
// NEIGHBOR_REQ - no data
// NEIGHBOR_REQ_RESP - 0 for negative, 1 for affirmative
// LSA - LSA data
// LSA_ACK - type long, sequence number
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

struct full_addr {
	struct in_addr addr;
	unsigned int port;
};

struct neighbour {
	struct in_addr id;
	unsigned int port;
	struct list_head list;
};

struct alive_control_struct {
	pid_t pid_of_control_thread;
	int num_unacked_messages;
	struct in_addr n_addr;
	int n_port;
};

struct link_cost_record {
	struct timespec time_out;
	struct timespec time_in;
	struct list_head list;
};

struct cost_control_struct {
	pid_t pid_of_control_thread;
	struct in_addr n_addr;
	int n_port;
	struct link_cost_record *lcr_list;
	long link_avg_nsec;
};

struct rt_entry {
	struct in_addr to_addr;
	struct in_addr thru_addr;
	int thru_port;
};

#endif // _ROUTER_DEFS_
