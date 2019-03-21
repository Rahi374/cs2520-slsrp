#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <arpa/inet.h>
#include <netinet/in.h>

#include "list.h"
#include "lsa.h"
#include "router.h"
#include "tools.h"

void print_lsa(struct lsa *lsa)
{
	int i;

	printf("\tLSA - %s:%u\n", inet_ntoa(lsa->router_id.addr), lsa->router_id.port);
	printf("\tseq = %ld, age = %d\n", lsa->seq, lsa->age);
	printf("\t%d entries:\n", lsa->nentries);

	for (i = 0; i < lsa->nentries; i++)
		printf("\t\t%s:%u - %ld\n",
			inet_ntoa(lsa->lsa_entry_list[i].neighbour_id.addr),
			lsa->lsa_entry_list[i].neighbour_id.port,
			lsa->lsa_entry_list[i].link_cost);
}

int lsa_is_valid(struct lsa *new_lsa, struct lsa *old_lsa)
{
	if (new_lsa->seq > old_lsa->seq)
		return 1;

	return 0;
}

struct lsa *extract_lsa(struct packet *packet)
{
	struct lsa *lsa = packet->data;
	int i;
	int count = 0;

	// i hope this works
	lsa->lsa_entry_list = (struct lsa_entry *)(((char *)packet->data) + sizeof(struct lsa));

	return lsa;
}

struct lsa *copy_lsa(struct lsa *lsa)
{
	struct lsa *lsa_copy = malloc(sizeof(struct lsa));
	memcpy(lsa_copy, lsa, sizeof(struct lsa));

	struct lsa_entry *lsa_entries = calloc(lsa->nentries, sizeof(struct lsa_entry));
	lsa_copy->lsa_entry_list = lsa_entries;
	memcpy(lsa_copy->lsa_entry_list, lsa->lsa_entry_list,
	       lsa->nentries * sizeof(struct lsa_entry));

	return lsa_copy;
}

void free_lsa(struct lsa *lsa)
{
	free(lsa->lsa_entry_list);
	free(lsa);
}

struct lsa_sending_entry *
realloc_lsa_sending_list(struct lsa_sending_entry *lsa_sending_list, int n)
{
	if (lsa_sending_list)
		free(lsa_sending_list);
	lsa_sending_list = calloc(n, sizeof(struct lsa_sending_entry));
	memset(lsa_sending_list, 0, n * sizeof(struct lsa_sending_entry));

	return lsa_sending_list;
}

void populate_lsa_sending_list_neighbours(struct lsa_control_struct *con_struct, int n)
{
	struct neighbour *ptr;
	int i;

	i = 0;
	list_for_each_entry(ptr, &neighbours_list->list, list) {
		if (i >= n)
			break;
		con_struct->lsa_sending_list[i].addr.addr.s_addr = ptr->id.s_addr;
		con_struct->lsa_sending_list[i].addr.port = ptr->port;
		con_struct->lsa_sending_list[i].s = 0;
		con_struct->lsa_sending_list[i].a = 0;
		i++;
	}
}

int send_lsa(struct lsa *lsa, struct full_addr *addr)
{
	void *data;
	int data_len;

	int sock = socket(AF_INET,SOCK_STREAM, 0);
	if (sock < 0) {
		perror("Error making socket\n");
		return -1;
	}

	struct sockaddr_in sa;
	memset(&sa, 0 ,sizeof(sa));
	sa.sin_addr.s_addr = addr->addr.s_addr;
	sa.sin_port = addr->port;
	sa.sin_family = AF_INET;

	int con = connect(sock, (struct sockaddr *)&sa, sizeof(sa));
	if (con < 0) {
		perror("error on connect in writer");
		return -1;
	}

	struct packet_header header;
	memset(&header, 0, sizeof(struct packet_header));
	header.source_addr.s_addr = cur_router_id.s_addr;
	header.source_port = cur_router_port;
	header.destination_addr.s_addr = addr->addr.s_addr;
	header.destination_port = addr->port;
	header.packet_type = LSA;

	data_len = sizeof(struct lsa) + sizeof(struct lsa_entry) * lsa->nentries;
	data = malloc(data_len);
	memcpy(data, lsa, sizeof(struct lsa));
	// i hope this works too
	memcpy((char *)data + sizeof(struct lsa), lsa->lsa_entry_list,
	       sizeof(struct lsa_entry) * lsa->nentries);

	header.length = data_len;
	header.checksum_header = checksum_header(&header);

	write_header_and_data(sock, &header, data, data_len);

	free(data);
	return 0;
}

int send_lsa_ack(struct lsa_ack *ack, struct full_addr *addr)
{
	int sock = socket(AF_INET,SOCK_STREAM, 0);
	if (sock < 0) {
		perror("Error making socket\n");
		return -1;
	}

	struct sockaddr_in sa;
	memset(&sa, 0 ,sizeof(sa));
	sa.sin_addr.s_addr = addr->addr.s_addr;
	sa.sin_port = addr->port;
	sa.sin_family = AF_INET;

	int con = connect(sock, (struct sockaddr *)&sa, sizeof(sa));
	if (con < 0) {
		perror("error on connect in writer");
		return -1;
	}

	struct packet_header header;
	memset(&header, 0, sizeof(struct packet_header));
	header.source_addr.s_addr = cur_router_id.s_addr;
	header.source_port = cur_router_port;
	header.destination_addr.s_addr = addr->addr.s_addr;
	header.destination_port = addr->port;
	header.packet_type = LSA_ACK;
	header.length = sizeof(struct lsa_ack);
	header.checksum_header = checksum_header(&header);

	write_header_and_data(sock, &header, ack, sizeof(struct lsa_ack));

	return 0;

}
