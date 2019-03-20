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
	while (1) {
		dprintf("alive thread proc\n");
		pthread_mutex_lock(&mutex_hm_alive);
		struct alive_control_struct *con_struct = (struct alive_control_struct*)lookup(hm_alive, n_router_id);
		if (con_struct == 0)
			printf("Error: did not find con_struct");
		con_struct->pid_of_control_thread = getpid();
		if (con_struct->num_unacked_messages > MAX_UNACKED_ALIVE_MESSAGES) {
			delete(hm_alive, n_router_id); 
			//TODO remove neighbor from neighbor list
			free(id);
			free(con_struct);
			printf("killing alive thread since num unacked too high\n");
			fflush(stdout);
			pthread_exit(0);
		} else {
			send_alive_msg(con_struct);				
			con_struct->num_unacked_messages++;
		}
		pthread_mutex_unlock(&mutex_hm_alive);
		usleep(1000000);

	}
}

void *add_neighbour_thread(void *id)
{
	dprintf("starting neighbour thread\n");

	struct full_addr *n_router_full_id = (struct full_addr *)id;
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
	sa.sin_port = n_router_full_id->port;
	sa.sin_addr.s_addr = n_router_full_id->addr.s_addr;
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
			if (ptr->id.s_addr == n_router_full_id->addr.s_addr)
				goto free;
		}
		pthread_mutex_unlock(&mutex_neighbours_list);

		dprintf("sending neighbour request\n");
		// send neighbour request
		struct packet_header header;
		header.packet_type = NEIGHBOR_REQ;
		header.length = 0;
		header.destination_addr.s_addr = n_router_full_id->addr.s_addr;
		header.destination_port = n_router_full_id->port;
		header.source_addr.s_addr = cur_router_id.s_addr;
		header.source_port = cur_router_port;

		write_header_and_data(sock, &header, 0, 0);

		dprintf("sent neighbour request\n");

		usleep(2000000);
	}

die:
	printf("failed to be friends with neighbour %s\n",
		inet_ntoa(n_router_full_id->addr));
free:
	free(id);
	close(sock);
}

void *lsa_sending_thread(void *id)
{
	struct lsa_control_struct *con_struct = (struct lsa_control_struct *)id;
	struct lsa *last_lsa;
	int i, n;

	dprintf("starting lsa sending thread for router id %s\n",
		inet_ntoa(con_struct->router_id->addr));

	pthread_mutex_lock(&con_struct->lock);
	con_struct->pid_of_control_thread = getpid();
	last_lsa = con_struct->lsa;
	n = con_struct->nentries;
	pthread_mutex_unlock(&con_struct->lock);

	while (1) {
		// loop through lsa sending table, send lsa and populate s
		for (i = 0; i < n; i++) {
			pthread_mutex_lock(&con_struct->lock);

			if (last_lsa != con_struct->lsa) {
				last_lsa = con_struct->lsa;
				i = 0;
			}

			if (con_struct->lsa_sending_list[i].a) {
				pthread_mutex_unlock(&con_struct->lock);
				continue;
			}

			// skip if this neighbour sent us the lsa
			if (con_struct->lsa_sending_list[i].addr.addr.s_addr ==
			    con_struct->origin_neighbour->addr.s_addr) {
				pthread_mutex_unlock(&con_struct->lock);
				continue;
			}

			send_lsa(last_lsa, &con_struct->lsa_sending_list[i].addr);
			con_struct->lsa_sending_list[i].s = 1;

			// hope this doesn't hit the edge case
			n = con_struct->nentries;
			pthread_mutex_unlock(&con_struct->lock);

			usleep(1000000);
		}

		// TODO get this from config?
		usleep(3000000);
	}
}

void *lsa_generating_thread(void *id)
{
	// malloc lsa and lsa entry list

	// get link costs, populate lsa

	// assemble the lsa, spawn lsa sending thread (see handle_lsa_packet())
}
