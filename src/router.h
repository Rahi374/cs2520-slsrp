#ifndef _ROUTER_DEFS_
#define _ROUTER_DEFS_

// port that every router listens on
#define LISTEN_PORT 50500

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

struct packet_header {
    unsigned int source_id;
    unsigned int destination_id;
    enum packet_type packet_type;
    int length;
    unsigned int checksum_header;
    unsigned int checksum_data;
};

#endif
