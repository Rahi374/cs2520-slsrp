#ifndef _ROUTER_DEFS_
#define _ROUTER_DEFS_

// port that every router listens on
#define LISTEN_PORT 50500

pthread_mutex_t mutex_hm_neighbours = PTHREAD_MUTEX_INITIALIZER;
struct table *hm_neighbours;

enum packet_type {
	NAK,
	ACK,
	UI_CONTROL,
	NEIGHBOR_REQ,
	NEIGHBOR_REQ_RESP,
	LSA,
	ALIVE,
	LINK_COST,
	LINK_DOWN,
};

// depending on the packet type, the length might simply contain the data
// NEIGHBOR_REQ - no data
// NEIGHBOR_REQ_RESP - 0 for negative, 1 for affirmative
struct packet_header {
	unsigned int source_id;
	unsigned int destination_id;
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

int has_data(struct packet_header *header)
{
	// TODO
	return 0;
}

unsigned int checksum_header(struct packet_header *header)
{
	// TODO
	return 0;
}

#endif // _ROUTER_DEFS_
