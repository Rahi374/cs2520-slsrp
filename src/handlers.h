#ifndef _HANDLERS_H_
#define _HANDLERS_H_

#include "router.h"

void handle_neighbour_req_packet(struct packet *packet);
void handle_neighbour_resp_packet(struct packet *packet);
void handle_test_packet(struct packet *packet);

#endif // _HANDLERS_H_
