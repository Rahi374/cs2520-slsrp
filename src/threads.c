#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>

#include "hm.h"
#include "router.h"
#include "tools.h"

void *lc_thread(void *id)
{
	unsigned int n_router_id = *((unsigned int *)id);
}

void *alive_thread(void *id)
{
	
	dprintf("starting alive thread\n");
	unsigned int n_router_id = *((unsigned int *)id);
	while (1){
		dprintf("alive thread proc\n");
		pthread_mutex_lock(&mutex_hm_alive);
		struct alive_control_struct *con_struct = (struct alive_control_struct*)lookup(hm_alive, n_router_id);
		if (con_struct == 0)
			printf("Error: did not find con_struct");
		con_struct->pid_of_control_thread = getpid();
		if (con_struct->num_unacked_messages > MAX_UNACKED_ALIVE_MESSAGES){
			delete(hm_alive, n_router_id); 
			//TODO remove neighbor from neighbor list
			free(id);
			free(con_struct);
			printf("killing alive thread since num unacked too high\n");
			fflush(stdout);
			pthread_exit(0);
		}else{
			send_alive_msg(con_struct);				
			con_struct->num_unacked_messages++;
		}
		pthread_mutex_unlock(&mutex_hm_alive);
		usleep(1000000);

	}
}

void *add_neighbour_thread(void *id)
{
	printf("starting neighbour thread\n");

	struct add_neighbour_command *n_router_full_id = (struct add_neighbour_command *)id;
	struct neighbour *ptr;
	int i;
	int sock;
	int con;
	struct sockaddr_in sa;

	// initialize socket
	dprintf("initializing socker for add neighbour\n");
	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock < 0) {
		perror("error making socker");
		goto die;
	}

	dprintf("connecting to socket to add neighbour\n");
	sa.sin_port = n_router_full_id->neighbour_port;
	sa.sin_addr.s_addr = n_router_full_id->neighbour_addr.s_addr;
	sa.sin_family = AF_INET;

	con = connect(sock, (struct sockaddr *)&sa, sizeof(sa));
	if (con < 0) {
		perror("error making connection");
		goto die;
	}

	for (i = 0; i < 3; i++) {
		dprintf("try #%d to add neighbour\n", i);
		// if not in neighbours list
		pthread_mutex_lock(&mutex_neighbours_list);
		list_for_each_entry(ptr, &neighbours_list->list, list) {
			if (ptr->id.s_addr == n_router_full_id->neighbour_addr.s_addr)
				goto free;
		}
		pthread_mutex_unlock(&mutex_neighbours_list);

		dprintf("sending neighbour request\n");
		// send neighbour request
		struct packet_header header;
		header.packet_type = NEIGHBOR_REQ;
		header.length = 0;
		header.destination_addr.s_addr = n_router_full_id->neighbour_addr.s_addr;
		header.destination_port = n_router_full_id->neighbour_port;
		header.source_addr.s_addr = cur_router_id.s_addr;
		header.source_port = cur_router_port;

		write_header_and_data(sock, &header, 0, 0);

		dprintf("sent neighbour request\n");

		usleep(2000000);
	}

die:
	printf("failed to be friends with neighbour %s\n",
		inet_ntoa(n_router_full_id->neighbour_addr));
free:
	free(id);
	close(sock);
}
