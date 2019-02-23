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

static int socket_fd;

void *writer_func()
{
	int sock = socket(AF_INET,SOCK_STREAM, 0);
	if (sock < 0)
		perror("Error making socket in writer 1\n");
	socket_fd = sock;

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

void process_input(char *input)
{
	int inp = atoi(input); 
	if (inp == 5) {
		printf("Testing External Writer\n");
		pthread_t writer_thread;
		pthread_create(&writer_thread, NULL, writer_func, NULL); 
	}

}

void start_repl()
{
	while(1){
		printf("\nEnter a command:\n");
		printf("	1 - Add neighbor\n");
		printf("	2 - Remove neighbor\n");
		printf("	3 - Get Topology\n");
		printf("	4 - Buncha other stuff that isn't implemented\n");
		printf("	5 - test external writer\n");
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
