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


void* listener_func();
void* listener_loop();
void* writer_func();
void* writer_func2();

int main(int argc, char *argv[])
{
    //set up listen socket
    int listen_sock = socket(AF_INET, SOCK_STREAM, 0);
    if(listen_sock < 0){
	perror("Error making socket for listener");
    }else{
	printf("Initial socket for listener successful\n");
    }
     
    struct sockaddr_in sag;
    memset(&sag, 0, sizeof(sag));
    sag.sin_port = htons(LISTEN_PORT);
    sag.sin_addr.s_addr = inet_addr("0.0.0.0");
    sag.sin_family = AF_INET;
    bind(listen_sock, (struct sockaddr *)&sag,sizeof(sag));

    printf("creating listener\n");
    pthread_t listener_thread;
    pthread_create(&listener_thread, NULL, listener_func, &listen_sock); 

    usleep(2000000);
    
    printf("creating writer 1\n");
    pthread_t writer_thread;
    pthread_create(&writer_thread, NULL, writer_func, NULL); 

    printf("creating writer 2\n");
    pthread_t writer_thread2;
    pthread_create(&writer_thread2, NULL, writer_func2, NULL); 

    while(1){
	usleep(1000000);
    }
    return 0;
}


//this function loops on the socket accepting connections
//spawns thread to deal with connections
void* listener_func(int *listen_sock)
{
    int listen_ret = listen(*listen_sock, 100);
    if(listen_ret < 0){
	perror("Error in listener listen call");
    }else{
	printf("listener is listening\n");
    }
    while(1){
	struct sockaddr_in sa2;
	socklen_t sa2_size = sizeof(sa2);
	int accept_sock = accept(*listen_sock, (struct sockaddr *)&sa2, &sa2_size);
	int *socket_fd = (int*)malloc(sizeof(int));//TODO add to hm for freeing purposes
	*socket_fd = accept_sock;
	if(accept_sock < 0){
	    perror("Error in accept in listener");
	}else{
	    printf("connection made in listener\n"); 
	    pthread_t listener_loop_thread;
	    pthread_create(&listener_loop_thread, NULL, listener_loop, socket_fd); 

	}
    }
}


void* listener_loop(int *sock)
{
    while(1){
	char buf[80];
	// TODO read HEADER_SIZE
	int n = read(*sock, buf, 6);
	if (n == 0) {
		printf("disconnected\n");
		pthread_exit(0);
	} else if (n < 0) {
		printf("error in read\n");
		pthread_exit(-1);
	}

	printf("%d Received: %s\n", *sock, buf);
	fflush(stdout);
	//usleep(1000000);
    }
}

void* writer_func(){
    int sock = socket(AF_INET,SOCK_STREAM,0);
    if(sock < 0){
	perror("Error making socket in writer 1\n");
    }
    struct sockaddr_in sa;
    memset(&sa, 0 ,sizeof(sa));
    sa.sin_port = htons(LISTEN_PORT);
    sa.sin_addr.s_addr = inet_addr("127.0.1.1");
    sa.sin_family = AF_INET;
    printf("trying to connect writer 1\n");
    int con = connect(sock, (struct sockaddr *)&sa, sizeof(sa));
    if(con < 0){
	perror("error on connect in writer");
    }else{
	printf("writer 1 connected\n");
    }

    while(1){
	char *bufout = "Hello";
	int n = write(sock, bufout, strlen(bufout)+1);
	if(n < 0){
	    perror("error in write 1\n");
	}
	usleep(1000000);
    }
}

void* writer_func2()
{
    int sock = socket(AF_INET,SOCK_STREAM,0);
    if(sock < 0){
	perror("Error making socket in writer 2\n");
    }
    struct sockaddr_in sa;
    memset(&sa, 0 ,sizeof(sa));
    sa.sin_port = htons(LISTEN_PORT);
    sa.sin_addr.s_addr = inet_addr("127.0.1.1");
    sa.sin_family = AF_INET;
    printf("trying to connect writer 2\n");
    int con = connect(sock, (struct sockaddr *)&sa, sizeof(sa));
    if(con < 0){
	perror("error on connect in writer");
    }else{
	printf("writer 2 connected\n");
    }

    while(1){
	char *bufout = "Mello";
	int n = write(sock, bufout, strlen(bufout)+1);
	if(n < 0){
	    perror("error in write 2\n");
	}
	usleep(2000000);
    }
}
