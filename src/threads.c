#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

#include "lib/db.h"
#include "hm.h"
#include "lsa.h"
#include "router.h"
#include "tools.h"

void timespec_diff(struct timespec *start, struct timespec *stop, struct timespec *result)
{
    if ((stop->tv_nsec - start->tv_nsec) < 0) {
        result->tv_sec = stop->tv_sec - start->tv_sec - 1;
        result->tv_nsec = stop->tv_nsec - start->tv_nsec + 1000000000;
    } else {
        result->tv_sec = stop->tv_sec - start->tv_sec;
        result->tv_nsec = stop->tv_nsec - start->tv_nsec;
    }
    return;
}

int send_alive_msg(struct alive_control_struct *con_struct)
{

	int sock = socket(AF_INET,SOCK_STREAM, 0);
	if (sock < 0) {
		perror("Error making socket\n");
		return -1;
	}

	struct sockaddr_in sa;
	memset(&sa, 0 ,sizeof(sa));
	sa.sin_port = con_struct->n_port;
	sa.sin_addr.s_addr = con_struct->n_addr.s_addr;
	sa.sin_family = AF_INET;

	int con = connect(sock, (struct sockaddr *)&sa, sizeof(sa));
	if (con < 0) {
		perror("error on connect in send alive");
		return -1;
	}

	struct packet_header alive_packet;
	memset(&alive_packet, 0, sizeof(struct packet_header));
	alive_packet.source_addr.s_addr = cur_router_id.s_addr;
	alive_packet.source_port = cur_router_port;
	alive_packet.destination_addr.s_addr = con_struct->n_addr.s_addr;
	alive_packet.destination_port = con_struct->n_port;
	alive_packet.packet_type = ALIVE;
	alive_packet.length = 0;
	alive_packet.checksum_header = checksum_header(&alive_packet);

	write(sock, &alive_packet, sizeof(struct packet_header));
	close(sock);
	return 0;
}

int send_link_cost_message(struct cost_control_struct *con_struct, struct link_cost_record *lcr)
{
	int sock = socket(AF_INET,SOCK_STREAM, 0);
	if (sock < 0) {
		perror("Error making socket\n");
		return -1;
	}

	struct sockaddr_in sa;
	memset(&sa, 0 ,sizeof(sa));
	sa.sin_port = con_struct->n_port;
	sa.sin_addr.s_addr = con_struct->n_addr.s_addr;
	sa.sin_family = AF_INET;

	int con = connect(sock, (struct sockaddr *)&sa, sizeof(sa));
	if (con < 0) {
		perror("error on connect in send lc");
		return -1;
	}

	struct packet_header lc_packet;
	memset(&lc_packet, 0, sizeof(struct packet_header));
	lc_packet.source_addr.s_addr = cur_router_id.s_addr;
	lc_packet.source_port = cur_router_port;
	lc_packet.destination_addr.s_addr = con_struct->n_addr.s_addr;
	lc_packet.destination_port = con_struct->n_port;
	lc_packet.packet_type = LINK_COST;
	lc_packet.length = sizeof(struct timespec);
	lc_packet.checksum_header = checksum_header(&lc_packet);

	write_header_and_data(sock, &lc_packet, &(lcr->time_out), sizeof(struct timespec));
	close(sock);
	return 0;
}

void *lc_thread(void *id)
{
	dprintf("starting lc thread\n");
	unsigned int n_router_id = *((unsigned int *)id);
	struct link_cost_record *ptr;
	struct link_cost_record *ptr_temp;

	pthread_mutex_lock(&mutex_hm_cost);
	struct cost_control_struct *con_struct = (struct cost_control_struct*)lookup(hm_cost, n_router_id);
	if (con_struct == 0){
		printf("Error: did not find initial cost con_struct");
		fflush(stdout);
		pthread_mutex_unlock(&mutex_hm_cost);
		return (void*)0;
	}
	con_struct->pid_of_control_thread = getpid();
	pthread_mutex_unlock(&mutex_hm_cost);

	struct timespec cur_time;
	struct timespec time_diff_timespec;
	while (1) {
		clock_gettime(CLOCK_MONOTONIC, &cur_time); 
		pthread_mutex_lock(&mutex_hm_cost);
		struct cost_control_struct *con_struct = (struct cost_control_struct*)lookup(hm_cost, n_router_id);
		if (con_struct == 0){
			printf("Error: did not find cost con_struct");
			fflush(stdout);
			pthread_mutex_unlock(&mutex_hm_cost);
			return (void*)0;
		}
		//loop checking times and send one new msg

		list_for_each_entry_safe(ptr, ptr_temp, &(con_struct->lcr_list->list), list) {
			if (ptr->time_out.tv_nsec != -1) {
				timespec_diff(&(ptr->time_out), &cur_time, &time_diff_timespec);
				if (time_diff_timespec.tv_sec >= 60) {
					//remove node from consideration
					//prob got lost somewhere, not counting it in link cost
					list_del(&(ptr->list));
				} else if (ptr->time_in.tv_nsec != -1) {
					//node (msg) has been acked and needs processed/removed
					long new_ns = (time_diff_timespec.tv_sec*1000000000)+time_diff_timespec.tv_nsec;
					con_struct->link_avg_nsec = (long)((0.9)*con_struct->link_avg_nsec)+(long)((0.1)*new_ns);
					list_del(&(ptr->list));
				}
			}
		}
		struct link_cost_record *single_lcr;
		single_lcr = malloc(sizeof(struct link_cost_record)); //TODO this needs freed
		clock_gettime(CLOCK_MONOTONIC, &cur_time); 
		memcpy(&(single_lcr->time_out), &cur_time, sizeof(struct timespec));
		single_lcr->time_in.tv_nsec = -1;
		//list_add(single_lcr, con_struct->lcr_list);
		list_add_tail(&(single_lcr->list), &(con_struct->lcr_list->list));
		//assume this call succeeds, if it doesnt it is removed after 60 seconds
		send_link_cost_message(con_struct, single_lcr);
		pthread_mutex_unlock(&mutex_hm_cost);
		dprintf("Sent lc msg to neighbor\n");
		//TODO make this value based on config values
		usleep(2000000);
	}
}

void *alive_thread(void *id)
{
	dprintf("starting alive thread\n");
	unsigned int n_router_id = *((unsigned int *)id);

	pthread_mutex_lock(&mutex_hm_alive);
	struct alive_control_struct *con_struct = (struct alive_control_struct*)lookup(hm_alive, n_router_id);
	if (con_struct == 0){
		printf("Error: did not find initial alive con_struct");
		fflush(stdout);
		pthread_mutex_unlock(&mutex_hm_alive);
		return (void*)0;
	}
	con_struct->pid_of_control_thread = getpid();
	pthread_mutex_unlock(&mutex_hm_alive);

	while (1) {
		dprintf("alive thread proc\n");

		pthread_mutex_lock(&mutex_hm_alive);
		struct alive_control_struct *con_struct = (struct alive_control_struct*)lookup(hm_alive, n_router_id);
		if (con_struct == 0) {
			printf("Error: did not find alive con_struct");
			fflush(stdout);
			pthread_mutex_unlock(&mutex_hm_alive);
			return (void*)0;
		}
		con_struct->pid_of_control_thread = getpid();

		if (con_struct->num_unacked_messages > MAX_UNACKED_ALIVE_MESSAGES) {
			delete(hm_alive, n_router_id); 
			//TODO remove neighbor from neighbor list and kill neighbor in other things
			free(id);
			free(con_struct);
			pthread_mutex_unlock(&mutex_hm_alive);
			printf("killing alive thread since num unacked too high\n");
			fflush(stdout);
			pthread_exit(0);
		} else {
			send_alive_msg(con_struct);				
			con_struct->num_unacked_messages++;
		}
		pthread_mutex_unlock(&mutex_hm_alive);
		usleep(2000000);
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
			if (ptr->id.s_addr == n_router_full_id->addr.s_addr) {
				pthread_mutex_unlock(&mutex_neighbours_list);
				goto free;
			}
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
		inet_ntoa(con_struct->router_id.addr));

	pthread_mutex_lock(&con_struct->lock);
	con_struct->pid_of_control_thread = getpid();
	last_lsa = con_struct->lsa;
	n = con_struct->nentries;
	pthread_mutex_unlock(&con_struct->lock);

	while (1) {
		//dprintf("lsa sending loop, n = %d\n", n);
		// loop through lsa sending table, send lsa and populate s
		for (i = 0; i < n; i++) {
			//dprintf("should i send lsa to %dth neighbour?\n", i);
			pthread_mutex_lock(&con_struct->lock);

			if (last_lsa != con_struct->lsa) {
				last_lsa = con_struct->lsa;
				i = -1;
				pthread_mutex_unlock(&con_struct->lock);
				continue;
			}

			if (con_struct->lsa_sending_list[i].a) {
				pthread_mutex_unlock(&con_struct->lock);
				continue;
			}

			// skip if this neighbour sent us the lsa
			//dprintf("origin neighbour = %s,\n",
			//		inet_ntoa(con_struct->origin_neighbour.addr));
			//dprintf("going to send to %s\n",
			//		inet_ntoa(con_struct->lsa_sending_list[i].addr.addr));
			if (con_struct->lsa_sending_list[i].addr.addr.s_addr ==
			    con_struct->origin_neighbour.addr.s_addr) {
				pthread_mutex_unlock(&con_struct->lock);
				continue;
			}

			//dprintf("sending lsa %s\n",
			//		inet_ntoa(last_lsa->router_id.addr));
			//dprintf("to %s!\n",
			//		inet_ntoa(con_struct->lsa_sending_list[i].addr.addr));
			send_lsa(last_lsa, &con_struct->lsa_sending_list[i].addr);
			con_struct->lsa_sending_list[i].s = 1;
			//dprintf("done sending lsa!\n");

			// hope this doesn't hit the edge case
			n = con_struct->nentries;

			if (--last_lsa->age < 0)
				last_lsa->seq = 0;
			pthread_mutex_unlock(&con_struct->lock);

			usleep(1000000);
		}

		// TODO get this from config?
		usleep(3000000);

		pthread_mutex_lock(&con_struct->lock);
		n = con_struct->nentries;
		pthread_mutex_unlock(&con_struct->lock);
	}
}

void *lsa_generating_thread(void *id)
{
	struct lsa *new_lsa = NULL;
	struct lsa_entry *new_lsa_entry_list = NULL;
	struct lsa_control_struct *con_struct;
	long cost;
	int i;
	struct timespec cur_time;
	struct neighbour *ptr;

	dprintf("%s\n", __func__);

	usleep(3000000);

	// allocate and spawn lsa_sending_thread
	con_struct = malloc(sizeof(struct lsa_control_struct));
	pthread_mutex_init(&con_struct->lock, NULL);
	con_struct->router_id.addr.s_addr = cur_router_id.s_addr;
	con_struct->router_id.port = cur_router_port;
	pthread_mutex_lock(&mutex_neighbours_list);
	con_struct->nentries = neighbour_count;
	con_struct->lsa_sending_list = calloc(neighbour_count,
					      sizeof(struct lsa_sending_entry));
	populate_lsa_sending_list_neighbours(con_struct, neighbour_count);
	pthread_mutex_unlock(&mutex_neighbours_list);

	pthread_mutex_lock(&mutex_hm_lsa);
	insert(hm_lsa, cur_router_id.s_addr, con_struct);
	lsa_count++;
	pthread_mutex_unlock(&mutex_hm_lsa);

	pthread_t lsa_sending_t;
	dprintf("spawning lsa sending thread from %s\n", __func__);
	pthread_create(&lsa_sending_t, NULL, lsa_sending_thread, (void *)con_struct);

	while (1) {
		//dprintf("lsa generation loop!\n");
		pthread_mutex_lock(&mutex_neighbours_list);
		pthread_mutex_lock(&mutex_hm_cost);

		new_lsa = malloc(sizeof(struct lsa));
		new_lsa_entry_list = calloc(neighbour_count, sizeof(struct lsa_entry));
		new_lsa->lsa_entry_list = new_lsa_entry_list;
		new_lsa->nentries = neighbour_count;

		// get link costs, populate lsa
		i = 0;
		list_for_each_entry(ptr, &neighbours_list->list, list) {
			if (i >= new_lsa->nentries)
				break;
			dprintf("lsa generation loop! - adding neighbour\n");
			cost = ((struct cost_control_struct *)lookup(hm_cost, ptr->id.s_addr))->link_avg_nsec;
			new_lsa->lsa_entry_list[i].neighbour_id.addr.s_addr = ptr->id.s_addr;
			new_lsa->lsa_entry_list[i].neighbour_id.port = ptr->port;
			new_lsa->lsa_entry_list[i].link_cost = cost;
			i++;
		}

		new_lsa->router_id.addr.s_addr = cur_router_id.s_addr;
		new_lsa->router_id.port = cur_router_port;

		clock_gettime(CLOCK_MONOTONIC, &cur_time);
		new_lsa->seq = cur_time.tv_nsec + 1000000000 * cur_time.tv_sec;

		// give lsa to sending thread
		//dprintf("giving lsa to sending thread; nentries = %d\n", new_lsa->nentries);
		pthread_mutex_lock(&con_struct->lock);
		if (con_struct->lsa)
			free_lsa(con_struct->lsa);
		con_struct->lsa = new_lsa;
		con_struct->nentries = neighbour_count;
		if (con_struct->lsa_sending_list)
			con_struct->lsa_sending_list =
				realloc_lsa_sending_list(con_struct->lsa_sending_list,
							 neighbour_count);
		else
			con_struct->lsa_sending_list = calloc(neighbour_count,
					sizeof(struct lsa_sending_entry));

		populate_lsa_sending_list_neighbours(con_struct, neighbour_count);
		pthread_mutex_unlock(&con_struct->lock);

		pthread_mutex_unlock(&mutex_hm_cost);
		pthread_mutex_unlock(&mutex_neighbours_list);

		// TODO config?
		usleep(10000000);
	}
}

void *rt_building_thread(void *id)
{
	dprintf("rt building thread starting\n");
	usleep(20000000);//dont try to build table for first 20 seconds
	while (1) {
		dprintf("updating the rt\n");	
		update_rt();
		dprintf("done updating the rt\n");
		int n = lsa_count;
		if (n > 1) {
			int i;
			for(i = 0; i < lsa_count; i++){
				char buf[50];
				char *str = inet_ntoa(rt[i].to_addr);
				strcpy(buf, str);
				dprintf("---------for: %s send thru: %s\n", buf, inet_ntoa(rt[i].thru_addr));
			}
		}
		usleep(20000000);
	}
}
