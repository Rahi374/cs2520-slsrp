#ifndef _TOOLS_H
#define _TOOLS_H

struct packet_header;

int has_data(struct packet_header *header);

unsigned int checksum_header(struct packet_header *header);

#endif // _TOOLS_H
