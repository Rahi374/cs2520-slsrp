#ifndef _ROUTER_DEFS_
#define _ROUTER_DEFS_

#define PACKET_HEADER_SIZE 24 //idk what this will end up being

#define LISTEN_PORT 50500 //port that every router listens on

//packet types
#define UI_CONTROL 0
#define NEIGHBOR_REQUEST 1
#define NEIGHBOR_REQUEST_RESPONSE 2


struct packet_header {
    int source_id;
    int destination_id;
    int packet_type;
    int data;
};


#endif
