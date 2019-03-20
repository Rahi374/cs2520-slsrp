#ifndef _HANDLERS_H_
#define _HANDLERS_H_

struct packet;

void handle_neighbour_req_packet(struct packet *packet);
void handle_neighbour_resp_packet(struct packet *packet);
void handle_alive_packet(struct packet *packet);
void handle_alive_resp_packet(struct packet *packet);
void handle_lc_packet(struct packet *packet);
void handle_lc_resp_packet(struct packet *packet);
void handle_lsa_packet(struct packet *packet);
void handle_lsa_ack_packet(struct packet *packet);
void handle_ui_control_add_neighbour(struct packet *packet);
void handle_test_packet(struct packet *packet);

#endif // _HANDLERS_H_
