#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "hm.h"

#include "router.h"

#include "handlers.h"
#include "threads.h"
#include "tools.h"

void handle_neighbour_req_packet(struct packet *packet)
{
	struct packet_header resp;
	unsigned int *neighbour_router_id;

	// accept (or deny, but we want friends) neighbour request
	// -> send response
	memset(&resp, 0, sizeof(resp));
	resp.source_id = packet->header->destination_id;
	resp.destination_id = packet->header->source_id;
	resp.packet_type = NEIGHBOR_REQ_RESP;
	resp.length = 1;
	resp.checksum_header = checksum_header(&resp);
	// assume it succeeds
	write(packet->sock, &resp, sizeof(resp));

	// add to neighbour list
	pthread_mutex_lock(&mutex_hm_neighbours);
	insert(hm_neighbours, packet->header->source_id, 1);
	pthread_mutex_unlock(&mutex_hm_neighbours);

	// spawn AliveThread
	neighbour_router_id = malloc(sizeof(packet->header->source_id));
	*neighbour_router_id = packet->header->source_id;
	pthread_t alive_t;
	pthread_create(&alive_t, NULL, alive_thread, (void *)neighbour_router_id);

	// spawn LCthread
	neighbour_router_id = malloc(sizeof(packet->header->source_id));
	*neighbour_router_id = packet->header->source_id;
	pthread_t lc_t;
	pthread_create(&lc_t, NULL, lc_thread, (void *)neighbour_router_id);
}

void handle_neighbour_resp_packet(struct packet *packet)
{
	unsigned int *neighbour_router_id;

	// do nothing if neighbour doesn't want to be friends
	if (!packet->header->length)
		return;

	// add to neighbour list
	pthread_mutex_lock(&mutex_hm_neighbours);
	insert(hm_neighbours, packet->header->source_id, 1);
	pthread_mutex_unlock(&mutex_hm_neighbours);

	// spawn AliveThread
	neighbour_router_id = malloc(sizeof(packet->header->source_id));
	*neighbour_router_id = packet->header->source_id;
	pthread_t alive_t;
	pthread_create(&alive_t, NULL, alive_thread, (void *)neighbour_router_id);

	// spawn LCthread
	neighbour_router_id = malloc(sizeof(packet->header->source_id));
	*neighbour_router_id = packet->header->source_id;
	pthread_t lc_t;
	pthread_create(&lc_t, NULL, lc_thread, (void *)neighbour_router_id);
}

void handle_test_packet(struct packet *packet)
{
	printf("test packet received\n");
	fflush(stdout);
	printf("data length: %d\n", packet->header->length);
	fflush(stdout);
	printf("packet data:\n");
	fflush(stdout);
	printf("%s\n", (char*)packet->data);
	fflush(stdout);
	printf("done with data\n");

}
