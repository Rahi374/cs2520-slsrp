#ifndef _TOOLS_H
#define _TOOLS_H

#ifdef DEBUG
#define dprintf(...) do { fprintf(stdout, __VA_ARGS__); fflush(stdout); } while (0)
#else
#define dprintf(...) do {} while (0)
#endif

struct packet_header;

int has_data(struct packet_header *header);

unsigned int checksum_header(struct packet_header *header);

void concat_header_and_data(void *dest, struct packet_header *header, void *data_pointer, int data_size);

int write_header_and_data(int sock, struct packet_header *header, void *data_pointer, int data_size);

int read_all_bytes_from_socket(int sock, void *dest, int num_bytes);

int send_alive_msg(struct alive_control_struct *con_struct);

#endif // _TOOLS_H
