#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "hm.h"
#include "handlers.h"
#include "lsa.h"
#include "router.h"
#include "threads.h"
#include "tools.h"

void handle_neighbour_req_packet(struct packet *packet)
{
	struct packet_header resp;
	unsigned int *neighbour_router_id;
	struct neighbour *neighbour;
	struct neighbour *ptr;
	int sock;
	int con;
	struct sockaddr_in sa;

	dprintf("handling neighbour req packet\n");

	// check if already in neighbour list
	pthread_mutex_lock(&mutex_neighbours_list);
	list_for_each_entry(ptr, &neighbours_list->list, list) {
		if (ptr->id.s_addr == packet->header->source_addr.s_addr) {
			dprintf("already in neighbour list");
			pthread_mutex_unlock(&mutex_neighbours_list);
			return;
		}
	}
	pthread_mutex_unlock(&mutex_neighbours_list);

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
	neighbour_count++;
	pthread_mutex_unlock(&mutex_neighbours_list);

	dprintf("spawning alive thread\n");
	// spawn AliveThread
	neighbour_router_id = malloc(sizeof(packet->header->source_addr.s_addr));
	*neighbour_router_id = packet->header->source_addr.s_addr;

	struct alive_control_struct *control_struct_alive = malloc(sizeof(struct alive_control_struct));
	control_struct_alive->num_unacked_messages = 0;
	control_struct_alive->n_addr = packet->header->source_addr;
	control_struct_alive->n_port = packet->header->source_port;
	pthread_mutex_lock(&mutex_hm_alive);
	insert(hm_alive, packet->header->source_addr.s_addr, control_struct_alive);	
	pthread_mutex_unlock(&mutex_hm_alive);

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
	struct neighbour *ptr;

	dprintf("received neighbour response\n");

	// do nothing if neighbour doesn't want to be friends
	if (!packet->header->length)
		return;

	// check if already in neighbour list
	pthread_mutex_lock(&mutex_neighbours_list);
	list_for_each_entry(ptr, &neighbours_list->list, list) {
		if (ptr->id.s_addr == packet->header->source_addr.s_addr) {
			dprintf("already in neighbour list");
			pthread_mutex_unlock(&mutex_neighbours_list);
			return;
		}
	}
	pthread_mutex_unlock(&mutex_neighbours_list);

	dprintf("adding to neighbour list\n");
	// add to neighbour list
	pthread_mutex_lock(&mutex_neighbours_list);
	neighbour = malloc(sizeof(struct neighbour));
	neighbour->id.s_addr = packet->header->source_addr.s_addr;
	neighbour->port = packet->header->source_port;
	list_add_tail(&(neighbour->list), &(neighbours_list->list));
	neighbour_count++;
	pthread_mutex_unlock(&mutex_neighbours_list);

	// spawn AliveThread
	neighbour_router_id = malloc(sizeof(packet->header->source_addr.s_addr));
	*neighbour_router_id = packet->header->source_addr.s_addr;

	struct alive_control_struct *control_struct_alive = malloc(sizeof(struct alive_control_struct));
	control_struct_alive->num_unacked_messages = 0;
	control_struct_alive->n_addr = packet->header->source_addr;
	control_struct_alive->n_port = packet->header->source_port;
	pthread_mutex_lock(&mutex_hm_alive);
	insert(hm_alive, packet->header->source_addr.s_addr, control_struct_alive);	
	pthread_mutex_unlock(&mutex_hm_alive);

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
	close(sock);
}

void handle_alive_resp_packet(struct packet *packet)
{
	dprintf("handling alive response packet\n");
	pthread_mutex_lock(&mutex_hm_alive);
	struct alive_control_struct *con_struct = lookup(hm_alive, packet->header->source_addr.s_addr);
	if (!con_struct){
		printf("Error, did not find that neighbor's control struct for alive resp packet\n");
		fflush(stdout);
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
	close(sock);
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
		if ((ptr->time_out.tv_nsec == timespec_sent_back->tv_nsec) &&
		    (ptr->time_out.tv_sec == timespec_sent_back->tv_sec)) {
			memcpy(&(ptr->time_in), &cur_time, sizeof(struct timespec));
			break;
		}
	}
	pthread_mutex_unlock(&mutex_hm_cost);
}
	
void handle_lsa_packet(struct packet *packet)
{
	struct lsa_control_struct *con_struct;
	struct lsa *lsa_tmp = extract_lsa(packet);
	struct lsa *lsa;
	struct lsa_ack ack;

	// dprintf("=== %s\n", __func__);

	lsa = copy_lsa(lsa_tmp);
	lsa->age = lsa_initial_age;

	print_lsa(lsa);

	// if thread already exists for router, replace lsa and lsa sending list
	pthread_mutex_lock(&mutex_hm_lsa);
	con_struct = lookup(hm_lsa, lsa->router_id.addr.s_addr);
	pthread_mutex_unlock(&mutex_hm_lsa);
	if (con_struct) {
		pthread_mutex_lock(&mutex_neighbours_list);
		pthread_mutex_lock(&con_struct->lock);

		// validate lsa (based on seq)
		if (!lsa_is_valid(lsa, con_struct->lsa)) {
			// still have to ack
			ack.router_id.s_addr = lsa->router_id.addr.s_addr;
			ack.seq = lsa->seq;
			// this will fail only if a neighbour's death is handled at this moment
			send_lsa_ack(&ack, &con_struct->origin_neighbour);
			pthread_mutex_unlock(&con_struct->lock);
			pthread_mutex_unlock(&mutex_neighbours_list);
			free_lsa(lsa);
			return;
		}

		// need to save which neighbour gave us the lsa so we don't send it to them
		con_struct->origin_neighbour.addr.s_addr = packet->header->source_addr.s_addr;
		con_struct->origin_neighbour.port = packet->header->source_port;

		free_lsa(con_struct->lsa);
		con_struct->lsa = lsa;
		con_struct->nentries = neighbour_count;
		con_struct->lsa_sending_list =
			realloc_lsa_sending_list(con_struct->lsa_sending_list,
						 neighbour_count);
		populate_lsa_sending_list_neighbours(con_struct, neighbour_count);
		pthread_mutex_unlock(&con_struct->lock);
		pthread_mutex_unlock(&mutex_neighbours_list);
		goto send_ack;
	}

	// otherwise allocate and spawn lsa_sending_thread
	con_struct = malloc(sizeof(struct lsa_control_struct));
	pthread_mutex_init(&con_struct->lock, NULL);
	con_struct->router_id.addr.s_addr = lsa->router_id.addr.s_addr;
	con_struct->router_id.port = lsa->router_id.port;
	con_struct->lsa = lsa;
	pthread_mutex_lock(&mutex_neighbours_list);
	pthread_mutex_lock(&con_struct->lock);
	con_struct->nentries = neighbour_count;
	con_struct->lsa_sending_list = calloc(neighbour_count,
					      sizeof(struct lsa_sending_entry));
	populate_lsa_sending_list_neighbours(con_struct, neighbour_count);
	pthread_mutex_unlock(&con_struct->lock);
	pthread_mutex_unlock(&mutex_neighbours_list);

	// need to save which neighbour gave us the lsa so we don't send it to them
	con_struct->origin_neighbour.addr.s_addr = packet->header->source_addr.s_addr;
	con_struct->origin_neighbour.port = packet->header->source_port;

	pthread_mutex_lock(&mutex_hm_lsa);
	insert(hm_lsa, lsa->router_id.addr.s_addr, con_struct);
	lsa_count++;
	pthread_mutex_unlock(&mutex_hm_lsa);

	pthread_t lsa_sending_t;
	//dprintf("spawning lsa sending thread from %s\n", __func__);
	pthread_create(&lsa_sending_t, NULL, lsa_sending_thread, (void *)con_struct);

send_ack:
	ack.router_id.s_addr = lsa->router_id.addr.s_addr;
	ack.seq = lsa->seq;
	pthread_mutex_lock(&con_struct->lock);
	send_lsa_ack(&ack, &con_struct->origin_neighbour);
	pthread_mutex_unlock(&con_struct->lock);
}

void handle_lsa_ack_packet(struct packet *packet)
{
	struct lsa_control_struct *con_struct;
	struct lsa_ack *ack = packet->data;
	int i;

	// dprintf("=== ACK %s\n", __func__);

	// get control struct for the router id
	pthread_mutex_lock(&mutex_hm_lsa);
	con_struct = lookup(hm_lsa, ack->router_id.s_addr);
	pthread_mutex_unlock(&mutex_hm_lsa);

	if (!con_struct) {
		printf("WARNING received ack for lsa that we didn't send\n");
		fflush(stdout);
		return;
	}

	// compare seq num
	pthread_mutex_lock(&con_struct->lock);
	if (con_struct->lsa->seq != ack->seq) {
		pthread_mutex_unlock(&con_struct->lock);
		return;
	}

	// find the entry in the sending list and set a
	for (i = 0; i < con_struct->nentries; i++) {
		if (con_struct->lsa_sending_list[i].addr.addr.s_addr ==
		    ack->router_id.s_addr) {
			con_struct->lsa_sending_list[i].a = 1;
			break;
		}
	}
	pthread_mutex_unlock(&con_struct->lock);
}

int send_file_part(struct packet *packet, unsigned int file_id_ns, int num_sent, int data_size_to_send, int final_pack, int file_len, char *file_name)
{
	dprintf("sending file part\n");
	struct file_part_control_struct file_part;
	file_part.file_id = file_id_ns;
	file_part.num_bytes = data_size_to_send;
	file_part.part_num = num_sent;
	file_part.is_final = final_pack;
	file_part.file_length = file_len;
	file_part.source_addr.s_addr = cur_router_id.s_addr;
	file_part.source_port = cur_router_port;
	strcpy(file_part.file_name, file_name);
	void *pack_data = malloc(sizeof(struct file_part_control_struct)+data_size_to_send);
	//memcpy pack data
	memcpy(pack_data, &file_part, sizeof(struct file_part_control_struct));	
	memcpy(pack_data + sizeof(struct file_part_control_struct), packet->data+(num_sent*MAX_FILE_PART), data_size_to_send);
	dprintf("===== sending data %c\n", *((char *)packet->data));
	//look up neighbour to send to
	struct in_addr neighbour_to_send_to_addr;
	int neighbour_to_send_to_port;
	int rt_index = (intptr_t)lookup(hm_rt_index, packet->header->destination_addr.s_addr);
	if (rt_index == 0){
		printf("no entry found in rt for that destination\n");
		fflush(stdout);
		return -1;
	}
	rt_index--;//cause grossness
	neighbour_to_send_to_addr.s_addr = rt[rt_index].thru_addr.s_addr;
	neighbour_to_send_to_port = rt[rt_index].thru_port;
	//build pack_header
		
	dprintf("debug 1\n");
	//insert into hm
	pthread_mutex_lock(&mutex_hm_file_ack);
	int is_acked = 0;
	insert(hm_file_ack, file_id_ns, (void*)&is_acked);
	pthread_mutex_unlock(&mutex_hm_file_ack);
	//send	
	
	int sock = socket(AF_INET,SOCK_STREAM, 0);
	if (sock < 0) {
		perror("Error making socket\n");
		return -1;
	}

	struct sockaddr_in sa;
	memset(&sa, 0 ,sizeof(sa));
	sa.sin_port = neighbour_to_send_to_port;
	sa.sin_addr.s_addr = neighbour_to_send_to_addr.s_addr;
	sa.sin_family = AF_INET;

	int con = connect(sock, (struct sockaddr *)&sa, sizeof(sa));
	if (con < 0) {
		perror("error on connect in send file part");
		return -1;
	}

	struct packet_header f_packet;
	memset(&f_packet, 0, sizeof(struct packet_header));
	f_packet.source_addr.s_addr = cur_router_id.s_addr;
	f_packet.source_port = cur_router_port;
	f_packet.destination_addr.s_addr = packet->header->destination_addr.s_addr;
	f_packet.destination_port = packet->header->destination_port;
	f_packet.packet_type = FILE_TRANSFER;
	f_packet.length = sizeof(struct file_part_control_struct)+data_size_to_send;
	f_packet.checksum_header = checksum_header(&f_packet);
	
	dprintf("debug 2\n");
	write_header_and_data(sock, &f_packet, pack_data, f_packet.length);
	int timeout_counter = 0;	
	while (!is_acked) {
		timeout_counter++;
		if (timeout_counter >= 4000) {
			printf("file transfer send timed out\n");
			fflush(stdout);

			pthread_mutex_lock(&mutex_hm_file_ack);
			delete(hm_file_ack, file_id_ns);
			pthread_mutex_unlock(&mutex_hm_file_ack);
			
			return -1;
		}
		usleep(10000);
	}
	
	dprintf("debug 3\n");
	pthread_mutex_lock(&mutex_hm_file_ack);
	delete(hm_file_ack, file_id_ns);
	pthread_mutex_unlock(&mutex_hm_file_ack);
	
	close(sock);
	free(pack_data);
	return 1;

}

void send_file_transfer_ack(struct packet *packet, struct file_part_control_struct *f_part)
{

	struct in_addr neighbour_to_send_to_addr;
	int neighbour_to_send_to_port;

	int rt_index = (intptr_t)lookup(hm_rt_index, f_part->source_addr.s_addr);
	if (rt_index == 0){
		printf("no entry found in rt for that destination\n");
		fflush(stdout);
		return;
	}
	rt_index--;//cause grossness
	neighbour_to_send_to_addr.s_addr = rt[rt_index].thru_addr.s_addr;
	neighbour_to_send_to_port = rt[rt_index].thru_port;

	int sock = socket(AF_INET,SOCK_STREAM, 0);
	if (sock < 0) {
		perror("Error making socket\n");
		return;
	}

	struct sockaddr_in sa;
	memset(&sa, 0 ,sizeof(sa));
	sa.sin_port = neighbour_to_send_to_port;
	sa.sin_addr.s_addr = neighbour_to_send_to_addr.s_addr;
	sa.sin_family = AF_INET;

	int con = connect(sock, (struct sockaddr *)&sa, sizeof(sa));
	if (con < 0) {
		perror("error on connect in handle lc packet");
		return;
	}

	struct packet_header ft_ack_packet;
	memset(&ft_ack_packet, 0, sizeof(struct packet_header));
	ft_ack_packet.source_addr.s_addr = cur_router_id.s_addr;
	ft_ack_packet.source_port = cur_router_port;
	ft_ack_packet.destination_addr.s_addr = f_part->source_addr.s_addr;
	ft_ack_packet.destination_port = f_part->source_port;
	ft_ack_packet.packet_type = FILE_TRANSFER_ACK;
	ft_ack_packet.length = sizeof(unsigned int);
	ft_ack_packet.checksum_header = checksum_header(&ft_ack_packet);
	write_header_and_data(sock, &ft_ack_packet, &f_part->file_id, sizeof(unsigned int));
	close(sock);
}

void handle_file_transfer_packet(struct packet *packet)
{
	dprintf("handling packet for file transfer\n");
	if (packet->header->destination_addr.s_addr == cur_router_id.s_addr) {
		dprintf("file transfer packet... for me... storing\n");
		//store file part and send ack
		struct file_part_control_struct ft_part;
		memcpy(&ft_part, packet->data, sizeof(struct file_part_control_struct));
		if (ft_part.is_final == 1 && ft_part.part_num == 0) {
			dprintf("handle ft pack option 1\n");
			dprintf("filename: %s\n", ft_part.file_name);
			dprintf("uhh here\n");
			FILE *fp = fopen(ft_part.file_name, "wb");
			if (fp == NULL) {
				perror("err");
				dprintf("error on fopen\n");
				exit(0);
			}
			void *file_data = packet->data + sizeof(struct file_part_control_struct);
			dprintf("file_data: %p, packet->data: %p\n", file_data, packet->data);
			fwrite(file_data, 1, ft_part.num_bytes, fp);
			fclose(fp);
		} else if (ft_part.is_final) {
			dprintf("handle ft pack option 2\n");
			pthread_mutex_lock(&mutex_hm_file_build);
			void *file_data = lookup(hm_file_build, ft_part.file_id);
			if (file_data == 0){
				printf("error, no file buffer found");
				fflush(stdout);
				pthread_mutex_unlock(&mutex_hm_file_build);
				return;
			}

			memcpy(file_data+(MAX_FILE_PART*ft_part.part_num), packet->data+sizeof(struct file_part_control_struct), ft_part.num_bytes);
			FILE *fp = fopen(ft_part.file_name, "wb");
			fwrite(file_data, 1, ft_part.file_length, fp);
		
			delete(hm_file_build, ft_part.file_id);
			free(file_data);
			pthread_mutex_unlock(&mutex_hm_file_build);
			fclose(fp);
		} else if (ft_part.part_num == 0) {
			dprintf("handle ft pack option 3\n");
			void *file_data = malloc(ft_part.file_length);
			memcpy(file_data, packet->data+sizeof(struct file_part_control_struct), ft_part.num_bytes);
			pthread_mutex_lock(&mutex_hm_file_build);
			insert(hm_file_build, ft_part.file_id, file_data);
			pthread_mutex_unlock(&mutex_hm_file_build);
		} else {
			dprintf("handle ft pack option 4\n");
			//middle packet, store in buffer
			pthread_mutex_lock(&mutex_hm_file_build);
			void *file_data = lookup(hm_file_build, ft_part.file_id);
			if (file_data == 0){
				printf("error, no file buffer found");
				fflush(stdout);
				pthread_mutex_unlock(&mutex_hm_file_build);
				return;
			}
			memcpy(file_data+(MAX_FILE_PART*ft_part.part_num), packet->data+sizeof(struct file_part_control_struct), MAX_FILE_PART);
			pthread_mutex_unlock(&mutex_hm_file_build);
		}
		dprintf("about to send file transfer ack\n");
		send_file_transfer_ack(packet, &ft_part);
		dprintf("after send file transfer ack\n");
	} else {
		dprintf("file transfer packet... not for me... transfering\n");
		//forward packet
		struct in_addr neighbour_to_send_to_addr;
		int neighbour_to_send_to_port;

		int rt_index = (intptr_t)lookup(hm_rt_index, packet->header->destination_addr.s_addr);
		if (rt_index == 0){
			printf("no entry found in rt for that destination\n");
			fflush(stdout);
			return;
		}
		rt_index--;//cause grossness
		neighbour_to_send_to_addr.s_addr = rt[rt_index].thru_addr.s_addr;
		neighbour_to_send_to_port = rt[rt_index].thru_port;



		int sock = socket(AF_INET,SOCK_STREAM, 0);
		if (sock < 0) {
			perror("Error making socket\n");
			return;
		}

		struct sockaddr_in sa;
		memset(&sa, 0 ,sizeof(sa));
		sa.sin_port = neighbour_to_send_to_port;
		sa.sin_addr.s_addr = neighbour_to_send_to_addr.s_addr;
		sa.sin_family = AF_INET;

		int con = connect(sock, (struct sockaddr *)&sa, sizeof(sa));
		if (con < 0) {
			perror("error on connect in send file part");
			return;
		}

		write_header_and_data(sock, packet->header, packet->data, packet->header->length);

		close(sock);
	}
}

void handle_file_transfer_ack_packet(struct packet *packet)
{
	dprintf("handling packet for file transfer ack\n");
	if (packet->header->destination_addr.s_addr == cur_router_id.s_addr) {
		dprintf("file transfer ack packet... for me... storing\n");
		unsigned int f_id;
		memcpy(&f_id, packet->data, sizeof(unsigned int));
		pthread_mutex_lock(&mutex_hm_file_ack);
		int *ack_var = (int*)lookup(hm_file_ack, f_id);	
		*ack_var = 1;
		pthread_mutex_unlock(&mutex_hm_file_ack);
	} else {
		dprintf("file transfer ack packet... not for me... transfering\n");
		//forward packet
		struct in_addr neighbour_to_send_to_addr;
		int neighbour_to_send_to_port;

		int rt_index = (intptr_t)lookup(hm_rt_index, packet->header->destination_addr.s_addr);
		if (rt_index == 0){
			printf("no entry found in rt for that destination\n");
			fflush(stdout);
			return;
		}
		rt_index--;//cause grossness
		neighbour_to_send_to_addr.s_addr = rt[rt_index].thru_addr.s_addr;
		neighbour_to_send_to_port = rt[rt_index].thru_port;

		int sock = socket(AF_INET,SOCK_STREAM, 0);
		if (sock < 0) {
			perror("Error making socket\n");
			return;
		}

		struct sockaddr_in sa;
		memset(&sa, 0 ,sizeof(sa));
		sa.sin_port = neighbour_to_send_to_port;
		sa.sin_addr.s_addr = neighbour_to_send_to_addr.s_addr;
		sa.sin_family = AF_INET;

		int con = connect(sock, (struct sockaddr *)&sa, sizeof(sa));
		if (con < 0) {
			perror("error on connect in send file part");
			return;
		}

		write_header_and_data(sock, packet->header, packet->data, packet->header->length);
		close(sock);
	}
}

void handle_ui_control_send_file_packet(struct packet *packet)
{
	dprintf("received ui command to send file\n");
	void *orig_data_malloc = (void*)packet->data;
	int file_name_len = packet->header->var;
	char file_name[128];
	memcpy(file_name, packet->data, file_name_len);
	packet->data = packet->data + file_name_len;
	int file_len;
	memcpy(&file_len, packet->data, sizeof(int));
	packet->data = packet->data + sizeof(int);
	//packet->data now points to all of the data for the file
	dprintf("file read in, first char is %c\n", *((char *)packet->data));

	int num_sent_and_acked = 0;
	int num_packs_needed = (file_len + MAX_FILE_PART - 1) / MAX_FILE_PART;
	int last_pack_data_size = file_len % MAX_FILE_PART;
	dprintf("file parts: %d last part size %d\n", num_packs_needed, last_pack_data_size);
	dprintf("test debug print 000\n");
	if (last_pack_data_size == 0) {
		last_pack_data_size = MAX_FILE_PART;
	}
	dprintf("test debug print\n");
	dprintf("filename: %s\n", file_name);
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	unsigned int file_id_ns = (unsigned int)(ts.tv_nsec + 1000000000*ts.tv_sec);
	dprintf("starting sending of file\n");
	while (num_sent_and_acked < num_packs_needed-1) {
		int res = send_file_part(packet, file_id_ns, num_sent_and_acked, MAX_FILE_PART, 0, file_len, file_name);
		if (res == -1) {
			printf("error: couldnt send middle file part\n");
			fflush(stdout);
			return;
		}
		num_sent_and_acked++;
	}
	dprintf("sending final part\n");
	int res = send_file_part(packet, file_id_ns, num_sent_and_acked, last_pack_data_size, 1, file_len, file_name);
	if (res == -1) {
		printf("error: couldnt send final file part\n");		
		fflush(stdout);
		return;
	}
	dprintf("done sending file\n");	
}

void handle_ui_control_add_neighbour(struct packet *packet)
{
	dprintf("received ui command to add neighbour!\n");
	struct full_addr *input;
	input = malloc(sizeof(struct full_addr));
	if (!input) {
		dprintf("failed to allocate memory for add neighbour thread input\n");
		return;
	}
	memcpy(input, packet->data, sizeof(struct full_addr));

	dprintf("spawning thread to deal with add neighbour\n");
	pthread_t add_neighbour_t;
	pthread_create(&add_neighbour_t, NULL, add_neighbour_thread, (void *)input);
}

void handle_ui_control_get_rt_packet(struct packet *packet)
{
	dprintf("ui get rt request received\n");
	pthread_mutex_lock(&mutex_hm_lsa);
	void *rt_data = malloc(sizeof(struct rt_entry)*lsa_count);		
	memcpy(rt_data, rt, sizeof(struct rt_entry)*lsa_count);

	struct packet_header rt_packet;
	memset(&rt_packet, 0, sizeof(struct packet_header));
	rt_packet.source_addr.s_addr = cur_router_id.s_addr;
	rt_packet.source_port = cur_router_port;
	rt_packet.destination_addr.s_addr = packet->header->source_addr.s_addr;
	rt_packet.destination_port = packet->header->source_port;
	rt_packet.packet_type = UI_CONTROL_GET_RT;
	rt_packet.length = lsa_count*sizeof(struct rt_entry);
	rt_packet.var = lsa_count;
	rt_packet.checksum_header = checksum_header(&rt_packet);

	write_header_and_data(packet->sock, &rt_packet, rt_data, rt_packet.length);
	
	pthread_mutex_unlock(&mutex_hm_lsa);
	free(rt_data);
	close(packet->sock);
}

void handle_ui_control_get_neighbours_packet(struct packet *packet)
{
	dprintf("ui get neighbours list request recevied\n");
	struct neighbour *ptr;
	pthread_mutex_lock(&mutex_neighbours_list);

	struct in_addr *n_arr;
	if (neighbour_count > 0) {
		n_arr = malloc(neighbour_count * sizeof(struct in_addr));
		int entry_num = 0;
		list_for_each_entry(ptr, &neighbours_list->list, list) {
			memcpy(&n_arr[entry_num], &ptr->id, sizeof(struct in_addr));
			entry_num++;
		}
	}
	struct packet_header nl_packet;
	memset(&nl_packet, 0, sizeof(struct packet_header));
	nl_packet.source_addr.s_addr = cur_router_id.s_addr;
	nl_packet.source_port = cur_router_port;
	nl_packet.destination_addr.s_addr = packet->header->source_addr.s_addr;
	nl_packet.destination_port = packet->header->source_port;
	nl_packet.packet_type = UI_CONTROL_GET_NEIGHBOURS;
	nl_packet.length = neighbour_count * sizeof(struct in_addr);
	nl_packet.var = neighbour_count;
	nl_packet.checksum_header = checksum_header(&nl_packet);

	write_header_and_data(packet->sock, &nl_packet, n_arr, nl_packet.length);

	pthread_mutex_unlock(&mutex_neighbours_list);
	free(n_arr);
	close(packet->sock);
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




