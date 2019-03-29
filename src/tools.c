#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "router.h"

int has_data(struct packet_header *header)
{
	// TODO
	switch (header->packet_type) {
		case LSA:
		case LSA_ACK:
		case TEST_PACKET:
		case UI_CONTROL_ADD_NEIGHBOUR:
		case LINK_COST:
		case LINK_COST_RESP:
		case FILE_TRANSFER:
		case FILE_TRANSFER_ACK:
		case UI_CONTROL_SEND_FILE:
		case NEIGHBOR_REQ_RESP:
		case NEIGHBOR_REQ:
			return 1;
		default:
			return 0;
	}
}

unsigned int checksum_header(struct packet_header *header)
{
	// TODO
	return 0;
}

void concat_header_and_data(void *dest, struct packet_header *header, void *data_pointer, int data_size)
{
	memcpy(dest, header, sizeof(struct packet_header));
	memcpy((char*)dest + sizeof(struct packet_header), data_pointer, data_size);
}

int write_header_and_data(int sock, struct packet_header *header, void *data_pointer, int data_size)
{
	int pack_size = sizeof(struct packet_header) + data_size;
	void *pack = malloc(pack_size);
	memset(pack, 0, pack_size);
	concat_header_and_data(pack, header, data_pointer, data_size);
	int n = write(sock, pack, pack_size);
	free(pack);
	return n;
}

int read_all_bytes_from_socket(int sock, void *dest, int num_bytes)
{
	int bytes_read = 0;
	while(bytes_read < num_bytes){
		int b = read(sock, (char*)dest+bytes_read, (num_bytes-bytes_read));
		if (b <= 0)
			return b;
		bytes_read += b;
	}
	return bytes_read;
}






