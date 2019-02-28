#include <string.h>
#include <unistd.h>

#include "router.h"

int has_data(struct packet_header *header)
{
	// TODO
	switch (header->packet_type) {
		case TEST_PACKET:
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
