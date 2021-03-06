#include <errno.h>
#include <netinet/in.h>
#include <net/if.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>

#include <arpa/inet.h>
#include <ifaddrs.h>
#include <netdb.h>

#include "lib/hm.h"

#include "router.h"

#include "handlers.h"
#include "naming.h"
#include "threads.h"
#include "tools.h"

#define DELAY_PROB 10

int lsa_sending_interval_us;
int lsa_generating_interval_us;
int lc_sending_interval_us;
int alive_sending_interval_us;
int lsa_initial_age;

pthread_mutex_t mutex_neighbours_list = PTHREAD_MUTEX_INITIALIZER;
struct neighbour *neighbours_list;
int neighbour_count = 0;

pthread_mutex_t mutex_hm_alive = PTHREAD_MUTEX_INITIALIZER;
struct table *hm_alive;

pthread_mutex_t mutex_hm_cost = PTHREAD_MUTEX_INITIALIZER;
struct table *hm_cost;

pthread_mutex_t mutex_hm_lsa = PTHREAD_MUTEX_INITIALIZER;
struct table *hm_lsa;
int lsa_count = 0;

struct table *hm_rt_index;//note, this gives index+1
struct rt_entry *rt;

pthread_mutex_t mutex_hm_file_ack = PTHREAD_MUTEX_INITIALIZER;
struct table *hm_file_ack;

pthread_mutex_t mutex_hm_file_build = PTHREAD_MUTEX_INITIALIZER;
struct table *hm_file_build;

struct in_addr cur_router_id;
int cur_router_port;

int load_config(const char *config_fn)
{
	FILE *f;
	int ret;

	f = fopen(config_fn, "r");
	if (!f)
		return errno;

	ret = fscanf(f, "%d\n%d\n%d\n%d\n%d",
			&lsa_sending_interval_us,
			&lsa_generating_interval_us,
			&lc_sending_interval_us,
			&alive_sending_interval_us,
			&lsa_initial_age);
	ret = (ret == 5) ? 0 : -1;

	fclose(f);
	return ret;
}

// handler for a new packet
// this is itself a thread, so no need to spawn new threads
// to call the more higher level handlers
void *handle_packet(void *p)
{
	struct packet *packet = (struct packet *)p;
	void *orig_data;
	unsigned int seed = time(NULL);

	if (rand_r(&seed) % 100 < DELAY_PROB)
		usleep((rand_r(&seed) % 1000) * 10);

	switch (packet->header->packet_type) {
		case NEIGHBOR_REQ:
			handle_neighbour_req_packet(packet);
			break;
		case NEIGHBOR_REQ_RESP:
			handle_neighbour_resp_packet(packet);
			break;
		case ALIVE:
			handle_alive_packet(packet);
			break;
		case ALIVE_RESP:
			handle_alive_resp_packet(packet);
			break;
		case LINK_COST:
			handle_lc_packet(packet);
			break;
		case LINK_COST_RESP:
			handle_lc_resp_packet(packet);
			break;
		case LSA:
			handle_lsa_packet(packet);
			break;
		case LSA_ACK:
			handle_lsa_ack_packet(packet);
			break;
		case FILE_TRANSFER:
			handle_file_transfer_packet(packet);
			break;
		case FILE_TRANSFER_ACK:
			handle_file_transfer_ack_packet(packet);
			break;
		case UI_CONTROL_ADD_NEIGHBOUR:
			 handle_ui_control_add_neighbour(packet);
			 break;
		case UI_CONTROL_SEND_FILE:
			 orig_data = packet->data;
			 handle_ui_control_send_file_packet(packet);
			 break;
		case UI_CONTROL_GET_RT:
			 handle_ui_control_get_rt_packet(packet);
			 break;
		case UI_CONTROL_GET_NEIGHBOURS:
			 handle_ui_control_get_neighbours_packet(packet);
			 break;
		case TEST_PACKET:
			handle_test_packet(packet);
			break;
		default:
			break;
	}

	if (packet->header->packet_type == UI_CONTROL_SEND_FILE) {
		if (orig_data)
			free(orig_data);
	} else {
		if (packet->data)
			free(packet->data);
	}
	free(packet->header);
	free(packet);
}

// to deal with every new packet
// kills itself when connection closed
void *listener_dispatch(void *s)
{
	int sock = *((int *)s);
	struct packet_header header;
	struct packet *packet;
	void *data;
	int n;

	// read header
	n = read_all_bytes_from_socket(sock, &header, sizeof(header));
	if (n <= 0)
		goto exit;
	//dprintf("received %d packet from %s\n", header.packet_type, inet_ntoa(header.source_addr));
	// read data if necessary
	data = NULL;
	if (has_data(&header)) {
		data = malloc(header.length);
		dprintf("reading %d bytes of data from %s\n", header.length, inet_ntoa(header.source_addr));
		n = read_all_bytes_from_socket(sock, data, header.length);
		if (n <= 0)
			goto free_data;
	}

	// assemble packet to give to appropriate handler
	packet = malloc(sizeof(struct packet));
	packet->data = data;
	packet->header = malloc(sizeof(header));
	memcpy(packet->header, &header, sizeof(header));
	if (packet->header->packet_type == UI_CONTROL_GET_RT || packet->header->packet_type == UI_CONTROL_GET_NEIGHBOURS) {
		packet->sock = sock;
	} else {
		close(sock);
	}
	// baton pass
	pthread_t packet_handler_thread;
	pthread_create(&packet_handler_thread, NULL, handle_packet, (void *)packet);
	goto exit;

free_data:
	printf("data disconnected or error %d\n", n);
	free(data);
exit:
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
		pthread_t listener_dispatch_thread;
		pthread_create(&listener_dispatch_thread, NULL, listener_dispatch, socket_fd);
	}
}

int main(int argc, char *argv[])
{
	int listen_sock;
	struct ifreq ifr;
	int ret;

	if (argc < 3) {
		printf("you must specify router name, then config file\n");
		printf("you may also specify network device name after that\n");
		return 1;
	}

	if (ret = load_config(argv[2])) {
		fprintf(stderr, "failed to load config '%s': %s\n",
			argv[2], ret == -1 ? "err" : strerror(ret));
		return 1;
	}
	dprintf("loaded config: %d, %d, %d, %d, %d\n",
			lsa_sending_interval_us,
			lsa_generating_interval_us,
			lc_sending_interval_us,
			alive_sending_interval_us,
			lsa_initial_age);

	listen_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (listen_sock < 0) {
		perror("Error making socket for listener");
		return 1;
	}
	struct sockaddr_in sag;
	memset(&sag, 0, sizeof(sag));
	sag.sin_port = 0;
	//sag.sin_addr.s_addr = inet_addr("0.0.0.0");
	sag.sin_addr.s_addr = INADDR_ANY;
	sag.sin_family = AF_INET;
	bind(listen_sock, (struct sockaddr *)&sag, sizeof(sag));
	int sag_size = sizeof(sag);
	getsockname(listen_sock, (struct sockaddr *)&sag, &sag_size);

	// create neighbours list
	pthread_mutex_lock(&mutex_neighbours_list);
	if (!(neighbours_list = malloc(sizeof(struct neighbour))))
		goto free_n;
	INIT_LIST_HEAD((&neighbours_list->list));
	pthread_mutex_unlock(&mutex_neighbours_list);

	hm_alive = createTable(100);
	if (!hm_alive)
		goto free_hm_alive;

	hm_cost = createTable(100);
	if (!hm_cost)
		goto free_hm_cost;

	hm_lsa = createTable(100);
	/* TODO
	if (!hm_alive)
		goto free_hm_alive;
	*/
	hm_file_ack = createTable(100);
	hm_file_build = createTable(100);

	rt = 0;
	hm_rt_index = 0;

	pthread_t listener_thread;
	pthread_create(&listener_thread, NULL, listener_thread_func, &listen_sock); 

	ifr.ifr_addr.sa_family = AF_INET;
	if (argc >= 4)
		strncpy(ifr.ifr_name, argv[3], IFNAMSIZ-1);
	else
		strncpy(ifr.ifr_name, "eth0", IFNAMSIZ-1);
	ioctl(listen_sock, SIOCGIFADDR, &ifr);

	cur_router_port = sag.sin_port;
	cur_router_id.s_addr = ((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr.s_addr;
	printf("Router is set up and listening on port %d\n", sag.sin_port);
	printf("Router is set up and listening on ip %s\n", inet_ntoa(cur_router_id));
	fflush(stdout);
	register_router(argv[1], inet_ntoa(cur_router_id), cur_router_port);

	pthread_t lsa_generating_t;
	dprintf("spawning our lsa generating thread\n");
	pthread_create(&lsa_generating_t, NULL, lsa_generating_thread, 0);

	pthread_t rt_builder_t;
	dprintf("spawning our rt building thread\n");
	pthread_create(&rt_builder_t, NULL, rt_building_thread, 0);

	while (1)
		usleep(10000000);
free_hm_cost:	
	destroyTable(hm_alive);
free_hm_alive:
	free(neighbours_list);
free_n:
	pthread_mutex_unlock(&mutex_neighbours_list);

	return 0;
}
