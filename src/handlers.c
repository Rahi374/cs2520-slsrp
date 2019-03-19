#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "hm.h"
#include "handlers.h"
#include "router.h"
#include "threads.h"
#include "tools.h"

void handle_neighbour_req_packet(struct packet *packet)
{
	struct packet_header resp;
	unsigned int *neighbour_router_id;
	struct neighbour *neighbour;
	int sock;
	int con;
	struct sockaddr_in sa;

	dprintf("handling neighbour req packet\n");

	// initialize socket
	dprintf("initializing socker for respondingn to add neighbour\n");
	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock < 0) {
		perror("error making socker");
		return;
	}

	dprintf("connecting to socket to respond to add neighbour, port %u\n",
			packet->header->source_port);
	sa.sin_port = packet->header->source_port;
	sa.sin_addr.s_addr = packet->header->source_addr.s_addr;
	sa.sin_family = AF_INET;

	con = connect(sock, (struct sockaddr *)&sa, sizeof(sa));
	if (con < 0) {
		perror("error making connection");
		return;
	}

	// accept (or deny, but we want friends) neighbour request
	// -> send response
	memset(&resp, 0, sizeof(resp));
	resp.source_addr.s_addr = packet->header->destination_addr.s_addr;
	resp.source_port = packet->header->destination_port;
	resp.destination_addr.s_addr = packet->header->source_addr.s_addr;
	resp.destination_port = packet->header->source_port;
	resp.packet_type = NEIGHBOR_REQ_RESP;
	resp.length = 1;
	resp.checksum_header = checksum_header(&resp);
	// assume it succeeds
	write(sock, &resp, sizeof(resp));

	dprintf("adding to neighbour list\n");
	// add to neighbour list
	pthread_mutex_lock(&mutex_neighbours_list);
	dprintf("mallocing\n");
	neighbour = malloc(sizeof(struct neighbour));
	dprintf("adding to list\n");
	neighbour->id.s_addr = packet->header->source_addr.s_addr;
	neighbour->port = packet->header->source_port;
	list_add_tail(&(neighbour->list), &(neighbours_list->list));
	pthread_mutex_unlock(&mutex_neighbours_list);

	dprintf("spawning alive thread\n");
	// spawn AliveThread
	neighbour_router_id = malloc(sizeof(packet->header->source_addr.s_addr));
	*neighbour_router_id = packet->header->source_addr.s_addr;

	struct alive_control_struct *control_struct_alive = malloc(sizeof(struct alive_control_struct));
	control_struct_alive->num_unacked_messages = 0;
	control_struct_alive->n_addr = packet->header->source_addr;
	control_struct_alive->n_port = packet->header->source_port;
	insert(hm_alive, packet->header->source_addr.s_addr, control_struct_alive);	

	pthread_t alive_t;
	pthread_create(&alive_t, NULL, alive_thread, (void *)neighbour_router_id);

	dprintf("spawning lc thread\n");
	// spawn LCthread
	neighbour_router_id = malloc(sizeof(packet->header->source_addr.s_addr));
	*neighbour_router_id = packet->header->source_addr.s_addr;

	struct cost_control_struct *control_struct_cost = malloc(sizeof(struct cost_control_struct));
	control_struct_cost->n_addr = packet->header->source_addr;
	control_struct_cost->n_port = packet->header->source_port;
	struct link_cost_record *lcr;
	lcr = (struct link_cost_record*)malloc(sizeof(struct link_cost_record));
	lcr->time_out.tv_nsec = -1;//marker of the stupid null node needed
	lcr->time_in.tv_nsec = -1;
	control_struct_cost->lcr_list = lcr;
	INIT_LIST_HEAD((&control_struct_cost->lcr_list->list));
	insert(hm_cost, packet->header->source_addr.s_addr, control_struct_cost);

	pthread_t lc_t;
	pthread_create(&lc_t, NULL, lc_thread, (void *)neighbour_router_id);

	close(sock);
}

void handle_neighbour_resp_packet(struct packet *packet)
{
	unsigned int *neighbour_router_id;
	struct neighbour *neighbour;

	dprintf("received neighbour response\n");

	// do nothing if neighbour doesn't want to be friends
	if (!packet->header->length)
		return;

	// add to neighbour list
	pthread_mutex_lock(&mutex_neighbours_list);
	neighbour = malloc(sizeof(struct neighbour));
	neighbour->id.s_addr = packet->header->source_addr.s_addr;
	neighbour->port = packet->header->source_port;
	list_add_tail(&(neighbour->list), &(neighbours_list->list));
	pthread_mutex_unlock(&mutex_neighbours_list);

	// spawn AliveThread
	neighbour_router_id = malloc(sizeof(packet->header->source_addr.s_addr));
	*neighbour_router_id = packet->header->source_addr.s_addr;

	struct alive_control_struct *control_struct_alive = malloc(sizeof(struct alive_control_struct));
	control_struct_alive->num_unacked_messages = 0;
	control_struct_alive->n_addr = packet->header->source_addr;
	control_struct_alive->n_port = packet->header->source_port;
	insert(hm_alive, packet->header->source_addr.s_addr, control_struct_alive);	

	pthread_t alive_t;
	pthread_create(&alive_t, NULL, alive_thread, (void *)neighbour_router_id);

	// spawn LCthread
	neighbour_router_id = malloc(sizeof(packet->header->source_addr.s_addr));
	*neighbour_router_id = packet->header->source_addr.s_addr;

	struct cost_control_struct *control_struct_cost = malloc(sizeof(struct cost_control_struct));
	control_struct_cost->n_addr = packet->header->source_addr;
	control_struct_cost->n_port = packet->header->source_port;
	struct link_cost_record *lcr;
	lcr = malloc(sizeof(struct link_cost_record));
	lcr->time_out.tv_nsec = -1;//marker of the stupid null node needed
	lcr->time_in.tv_nsec = -1;
	control_struct_cost->lcr_list = lcr;
	INIT_LIST_HEAD((&control_struct_cost->lcr_list->list));
	insert(hm_cost, packet->header->source_addr.s_addr, control_struct_cost);

	pthread_t lc_t;
	pthread_create(&lc_t, NULL, lc_thread, (void *)neighbour_router_id);
}

void handle_alive_packet(struct packet *packet)
{
	dprintf("handling alive packet\n");
	int sock = socket(AF_INET,SOCK_STREAM, 0);
	if (sock < 0) {
		perror("Error making socket\n");
		return;
	}

	struct sockaddr_in sa;
	memset(&sa, 0 ,sizeof(sa));
	sa.sin_port = packet->header->source_port;
	sa.sin_addr.s_addr = packet->header->source_addr.s_addr;
	sa.sin_family = AF_INET;

	int con = connect(sock, (struct sockaddr *)&sa, sizeof(sa));
	if (con < 0) {
		perror("error on connect in writer");
		return;
	}

	struct packet_header resp;
	memset(&resp, 0, sizeof(resp));
	resp.source_addr = packet->header->destination_addr;
	resp.destination_addr = packet->header->source_addr;
	resp.packet_type = ALIVE_RESP;
	resp.length = 0;
	resp.checksum_header = checksum_header(&resp);
	write(sock, &resp, sizeof(resp));
}

void handle_alive_resp_packet(struct packet *packet)
{
	dprintf("handling alive response packet\n");
	pthread_mutex_lock(&mutex_hm_alive);
	struct alive_control_struct *con_struct = lookup(hm_alive, packet->header->source_addr.s_addr);
	if (!con_struct){
		printf("Error, did not find that neighbor's control struct for alive resp packet\n");
		return;
	}
	con_struct->num_unacked_messages = 0;
	pthread_mutex_unlock(&mutex_hm_alive);
}

void handle_lc_packet(struct packet *packet)
{
	dprintf("handling lc packet\n");
	int sock = socket(AF_INET,SOCK_STREAM, 0);
	if (sock < 0) {
		perror("Error making socket\n");
		return;
	}

	struct sockaddr_in sa;
	memset(&sa, 0 ,sizeof(sa));
	sa.sin_port = packet->header->source_port;
	sa.sin_addr.s_addr = packet->header->source_addr.s_addr;
	sa.sin_family = AF_INET;

	int con = connect(sock, (struct sockaddr *)&sa, sizeof(sa));
	if (con < 0) {
		perror("error on connect in handle lc packet");
		return;
	}

	struct packet_header lc_packet;
	memset(&lc_packet, 0, sizeof(struct packet_header));
	lc_packet.source_addr.s_addr = cur_router_id.s_addr;
	lc_packet.source_port = cur_router_port;
	lc_packet.destination_addr.s_addr = packet->header->source_addr.s_addr;
	lc_packet.destination_port = packet->header->source_port;
	lc_packet.packet_type = LINK_COST_RESP;
	lc_packet.length = sizeof(struct timespec);
	lc_packet.checksum_header = checksum_header(&lc_packet);
	write_header_and_data(sock, &lc_packet, packet->data, sizeof(struct timespec));
}

void handle_lc_resp_packet(struct packet *packet)
{
	dprintf("handling lc resp packet\n");
	struct link_cost_record *ptr;
	struct timespec cur_time;
	clock_gettime(CLOCK_MONOTONIC, &cur_time);
	unsigned int n_router_id = packet->header->source_addr.s_addr;	
	pthread_mutex_lock(&mutex_hm_cost);
	struct cost_control_struct *con_struct = (struct cost_control_struct*)lookup(hm_cost, n_router_id);
	if (con_struct == 0){
		printf("Error: did not find cost con_struct");
		fflush(stdout);
		pthread_mutex_unlock(&mutex_hm_cost);
		return;
	}
	struct timespec *timespec_sent_back = (struct timespec*)packet->data;
	//TODO does the data and packet need freed?
	list_for_each_entry(ptr, &(con_struct->lcr_list->list), list) {
		if ((ptr->time_out.tv_nsec == timespec_sent_back->tv_nsec)&&(ptr->time_out.tv_sec == timespec_sent_back->tv_sec)) {
			memcpy(&(ptr->time_in), &cur_time, sizeof(struct timespec));
			break;
		}
	}
	pthread_mutex_unlock(&mutex_hm_cost);
	
}

void handle_ui_control_add_neighbour(struct packet *packet)
{
	dprintf("received ui command to add neighbour!\n");
	struct add_neighbour_command *input;
	input = malloc(sizeof(struct add_neighbour_command));
	if (!input) {
		dprintf("failed to allocate memory for add neighbour thread input\n");
		return;
	}
	dprintf("memcpying\n");
	memcpy(input, packet->data, sizeof(struct add_neighbour_command));

	dprintf("spawning thread to deal with add neighbour\n");
	pthread_t add_neighbour_t;
	pthread_create(&add_neighbour_t, NULL, add_neighbour_thread, (void *)input);
}

void handle_test_packet(struct packet *packet)
{
	dprintf("test packet received\n");
	dprintf("data length: %d\n", packet->header->length);
	dprintf("ip src: %s\n", inet_ntoa(packet->header->source_addr));
	dprintf("packet data:\n");
	dprintf("%s\n", (char*)packet->data);
	dprintf("done with data\n");
}




