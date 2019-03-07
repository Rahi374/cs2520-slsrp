#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <arpa/inet.h>
#include <errno.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <errno.h>
#include <stdlib.h>

#include "router.h"
#include "tools.h"

char *router_ip;
int router_port;

//function for test purposes
void *writer_func()
{
	int sock = socket(AF_INET,SOCK_STREAM, 0);
	if (sock < 0)
		perror("Error making socket in writer 1\n");

	struct sockaddr_in sa;
	memset(&sa, 0 ,sizeof(sa));
	sa.sin_port = 0;
	sa.sin_addr.s_addr = inet_addr("172.18.0.2");
	sa.sin_family = AF_INET;

	printf("trying to connect writer 3\n");
	int con = connect(sock, (struct sockaddr *)&sa, sizeof(sa));
	if (con < 0)
		perror("error on connect in writer");
	else
		printf("writer 3 connected\n");

	while (1) {
		char *bufout = "Fello";
		int n = write(sock, bufout, strlen(bufout)+1);
		if(n < 0)
			perror("error in write 3\n");
		usleep(1000000);
	}
}

void add_neighbor()
{
	int counter;
	char neighbor_id_to[16];
	char neighbor_port_to[10];

	printf("Specify an ID to send a neighbor request to:\n");
	scanf("%s", neighbor_id_to);
	for(counter = 0; counter < 16; counter++){
		if(neighbor_id_to[counter] == 0){
			neighbor_id_to[counter] = '\0';
			break;
		}
	}
	neighbor_id_to[15] = '\0';
	printf("Specify a port for that neighbor:\n");
	scanf("%s", neighbor_port_to);
	for(counter = 0; counter < 10; counter++){
		if(neighbor_port_to[counter] == 0){
			neighbor_port_to[counter] = '\0';
			break;
		}
	}
	neighbor_port_to[9] = '\0';
	int neighbor_port_to_int = atoi(neighbor_port_to);

	int sock = socket(AF_INET,SOCK_STREAM, 0);
	if (sock < 0) {
		perror("Error making socket\n");
		return;
	}

	struct sockaddr_in sa;
	memset(&sa, 0 ,sizeof(sa));
	sa.sin_port = router_port;
	sa.sin_addr.s_addr = inet_addr(router_ip);
	sa.sin_family = AF_INET;

	int con = connect(sock, (struct sockaddr *)&sa, sizeof(sa));
	if (con < 0) {
		perror("error on connect in writer");
		return;
	}
	struct packet_header pack_header;
	pack_header.packet_type = UI_CONTROL_ADD_NEIGHBOUR;
	pack_header.length = sizeof(struct add_neighbour_command);
	inet_aton(router_ip, &pack_header.destination_addr);
	pack_header.destination_port = router_port;
	pack_header.source_addr = sa.sin_addr;
	pack_header.source_port = sa.sin_port;

	struct add_neighbour_command neighbour_struct;
	neighbour_struct.neighbour_port = (unsigned int)neighbor_port_to_int;
	inet_aton(neighbor_id_to, &neighbour_struct.neighbour_addr);
	int n = write_header_and_data(sock, &pack_header, &neighbour_struct, sizeof(struct add_neighbour_command));
	if (n < 0) {
		perror("error in write to socket\n");
		return;
	}
	//TODO read on the socket and wait for router to tell me success/fail and report that
}



void test_message()
{
	int counter;
	char neighbor_id_to[16];
	char neighbor_port_to[10];
	printf("Specify an ID to send a neighbor request to:\n");
	scanf("%s", neighbor_id_to);
	for(counter = 0; counter < 16; counter++){
		if(neighbor_id_to[counter] == 0){
			neighbor_id_to[counter] = '\0';
			break;
		}
	}
	neighbor_id_to[15] = '\0';
	printf("Specify a port for that neighbor:\n");
	scanf("%s", neighbor_port_to);
	for(counter = 0; counter < 10; counter++){
		if(neighbor_port_to[counter] == 0){
			neighbor_port_to[counter] = '\0';
			break;
		}
	}
	neighbor_port_to[9] = '\0';
	int neighbor_port_to_int = atoi(neighbor_port_to);

	int sock = socket(AF_INET,SOCK_STREAM, 0);
	if (sock < 0) {
		perror("Error making socket\n");
		return;
	}

	struct sockaddr_in sa;
	memset(&sa, 0 ,sizeof(sa));
	sa.sin_port = router_port;
	sa.sin_addr.s_addr = inet_addr(router_ip);
	sa.sin_family = AF_INET;

	int con = connect(sock, (struct sockaddr *)&sa, sizeof(sa));
	if (con < 0) {
		perror("error on connect in writer");
		return;
	}
	char *msg = "I am a msg of a different length";
	//fill packet
	struct packet_header pack_header;
	pack_header.packet_type = TEST_PACKET;
	pack_header.length = strlen(msg)+1;
	inet_aton(router_ip, &pack_header.destination_addr);
	pack_header.destination_port = router_port;
	pack_header.source_addr = sa.sin_addr;
	pack_header.source_port = sa.sin_port;
	int n = write_header_and_data(sock, &pack_header, msg, strlen(msg)+1);
	if (n < 0) {
		perror("error in write to socket\n");
		return;
	}
}


void remove_neighbor()
{
	int counter;
	printf("Specify an ID to remove:\n");
	char neighbor_id[16];
	scanf("%s", neighbor_id);
	for(counter = 0; counter < 16; counter++){
		if(neighbor_id[counter] == 0){
			neighbor_id[counter] = '\0';
			break;
		}
	}
	neighbor_id[15] = '\0';
	printf("TODO\n");
}

void get_topology()
{
	printf("TODO\n");
}

void get_routing_table()
{
	printf("TODO\n");
}

void test_external_writer()
{
	printf("Testing External Writer:\n");
	printf("	1 - Make thread to loop sending input\n");
	printf("	2 - Send neighbor request\n");
	char send_input[5];
	scanf("%s", send_input);
	if(atoi(send_input) == 1){
		pthread_t writer_thread;
		pthread_create(&writer_thread, NULL, writer_func, NULL);
	}else if(atoi(send_input)){
		add_neighbor();
	}
}

void process_input(char *input)
{
	int inp = atoi(input);
	switch(inp){
		case 0:
			test_message();
			break;
		case 1:
			add_neighbor();
			break;
		case 2:
			remove_neighbor();
			break;
		case 3:
			get_topology();
			break;
		case 4:
			get_routing_table();
			break;
		case 5:
			test_external_writer();
			break;
		case 6:
			break;

	}

}

void start_repl()
{
	while(1){
		printf("\nEnter a command:\n");
		printf("	0 - Test Message\n");
		printf("	1 - Add neighbor\n");
		printf("	2 - Remove neighbor\n");
		printf("	3 - Get Topology\n");
		printf("	4 - Get Routing Table\n");
		printf("	5 - test external writer\n");
		printf("	6 - Buncha other stuff that isn't implemented\n");
		char input[5];
		scanf("%s", input);
		process_input(input);
		usleep(100000);
	}
}

int main(int argc, char *argv[])
{
	if (argc < 3){
		printf("needs ip and port\n");
	}else{
		router_ip = argv[1];
		router_port = atoi(argv[2]);
		start_repl();
	}
	return 0;
}
