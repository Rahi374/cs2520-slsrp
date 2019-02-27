#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#include <pthread.h>

#include <arpa/inet.h>
#include <ifaddrs.h>
#include <netdb.h>

#include "lib/hm.h"

#include "router.h"

#include "handlers.h"
#include "threads.h"
#include "tools.h"

pthread_mutex_t mutex_hm_neighbours = PTHREAD_MUTEX_INITIALIZER;
struct table *hm_neighbours;

// handler for a new packet
// this is itself a thread, so no need to spawn new threads
// to call the more higher level handlers
void *handle_packet(void *p)
{
	struct packet *packet = (struct packet *)p;

	switch (packet->header->packet_type) {
		case NEIGHBOR_REQ:
			handle_neighbour_req_packet(packet);
			break;
		case NEIGHBOR_REQ_RESP:
			handle_neighbour_resp_packet(packet);
			break;
		default:
			break;
	}

	free(p);
}

// this function/thread loops on one connection and spawns threads
// to deal with every new packet
// kills itself when connection closed
void *listener_loop(void *s)
{
	int sock = *((int *)s);
	struct packet_header header;
	struct packet *packet;
	void *data;
	int n;

	// read header
	n = read(sock, &header, sizeof(header));
	if (n <= 0)
		goto exit;

	// read data if necessary
	data = NULL;
	if (has_data(&header)) {
		data = malloc(header.length);
		n = read(sock, data, header.length);
		if (n <= 0)
			goto free_data;
	}

	// assemble packet to give to appropriate handler
	packet = malloc(sizeof(struct packet));
	packet->data = data;
	packet->header = malloc(sizeof(header));
	memcpy(packet->header, &header, sizeof(header));
	packet->sock = sock;

	// baton pass
	pthread_t packet_handler_thread;
	pthread_create(&packet_handler_thread, NULL, handle_packet, (void *)packet);

free_data:
	printf("data disconnected or error %d\n", n);
	free(data);
exit:
	printf("header disconnected or error %d\n", n);
	free(s);
	pthread_exit(0);
}

// this function loops on the socket accepting connections
// spawns thread to deal with connections
void *listener_thread_func(void *ls)
{
	int *listen_sock = (int *)ls;
	struct sockaddr_in sa;
	socklen_t sa_size = sizeof(sa);
	int accept_sock;
	int *socket_fd;
	int ret;

	ret = listen(*listen_sock, 100);
	if (ret < 0) {
		perror("Error in listener listen call");
		exit(1);
	}

	while (1) {
		accept_sock = accept(*listen_sock, (struct sockaddr *)&sa, &sa_size);
		if (accept_sock < 0) {
			perror("Error in accept in listener");
			continue;
		}

		socket_fd = (int*)malloc(sizeof(int));
		*socket_fd = accept_sock;

		printf("connection made in listener: socket_fd = %d\n", *socket_fd);
		pthread_t listener_loop_thread;
		pthread_create(&listener_loop_thread, NULL, listener_loop, socket_fd);
	}
}

int main(int argc, char *argv[])
{
	int listen_sock;
	
	listen_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (listen_sock < 0) {
		perror("Error making socket for listener");
		return 1;
	}

	struct sockaddr_in sag;
	memset(&sag, 0, sizeof(sag));
	sag.sin_port = htons(LISTEN_PORT);
	sag.sin_addr.s_addr = inet_addr("0.0.0.0");
	sag.sin_family = AF_INET;
	bind(listen_sock, (struct sockaddr *)&sag, sizeof(sag));

	pthread_mutex_lock(&mutex_hm_neighbours);
	hm_neighbours = createTable(16);
	pthread_mutex_unlock(&mutex_hm_neighbours);

	// TODO spawn timer threads

	pthread_t listener_thread;
	pthread_create(&listener_thread, NULL, listener_thread_func, &listen_sock); 

	printf("Main is done\n");

	while (1)
		usleep(10000000);

	pthread_mutex_lock(&mutex_hm_neighbours);
	destroyTable(hm_neighbours);
	pthread_mutex_unlock(&mutex_hm_neighbours);

	return 0;
}
