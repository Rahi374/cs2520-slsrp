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

//function for test purposes
void *writer_func()
{
	int sock = socket(AF_INET,SOCK_STREAM, 0);
	if (sock < 0)
		perror("Error making socket in writer 1\n");

	struct sockaddr_in sa;
	memset(&sa, 0 ,sizeof(sa));
	sa.sin_port = htons(LISTEN_PORT);
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
	char neighbor_id_sender[16];
	char neighbor_id_to[16];
	printf("Specify an ID to send a neighbor request from:\n");
	scanf("%s", neighbor_id_sender);
	for(counter = 0; counter < 16; counter++){
		if(neighbor_id_sender[counter] == 0){
			neighbor_id_sender[counter] = '\0';
			break;
		}
	}
	neighbor_id_sender[15] = '\0';
	printf("Specify an ID to send a neighbor request to:\n");
	scanf("%s", neighbor_id_to);
	for(counter = 0; counter < 16; counter++){
		if(neighbor_id_to[counter] == 0){
			neighbor_id_to[counter] = '\0';
			break;
		}
	}
	neighbor_id_to[15] = '\0';
	int sock = socket(AF_INET,SOCK_STREAM, 0);
	if (sock < 0) {
		perror("Error making socket\n");
		return;
	}

	struct sockaddr_in sa;
	memset(&sa, 0 ,sizeof(sa));
	sa.sin_port = htons(LISTEN_PORT);
	sa.sin_addr.s_addr = inet_addr(neighbor_id_sender);
	sa.sin_family = AF_INET;

	int con = connect(sock, (struct sockaddr *)&sa, sizeof(sa));
	if (con < 0) {
		perror("error on connect in writer");
		return;
	}
	char *msg = "I am a msg of a different length";
	struct packet_header pack_header;
	pack_header.packet_type = TEST_PACKET;
	pack_header.length = strlen(msg)+1;
	//int pack_size = sizeof(struct packet_header)+strlen(msg)+1;
	//void *pack = malloc(pack_size);
	//concat_header_and_data(pack, &pack_header, msg, strlen(msg)+1);
	int n = write_header_and_data(sock, &pack_header, msg, strlen(msg)+1);
	if (n < 0) {
		perror("error in write to socket\n");
		return;
	}
	//TODO read on the socket and wait for router to tell me success/fail and report that
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
	if (argc == 1)
		start_repl();
	else
		printf("UI with command line args\n");

	return 0;
}
