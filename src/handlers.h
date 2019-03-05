#ifndef _HANDLERS_H_
#define _HANDLERS_H_

void handle_neighbour_req_packet(struct packet *packet);
void handle_neighbour_resp_packet(struct packet *packet);
void handle_alive_packet(struct packet *packet);
void handle_alive_resp_packet(struct packet *packet);
void handle_test_packet(struct packet *packet);

#endif // _HANDLERS_H_
