#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "router.h"
#include "lib/hm.h"

void *lc_thread(void *id)
{
	unsigned int n_router_id = *((unsigned int *)id);
}

void *alive_thread(void *id)
{
	printf("starting alive thread\n");
	unsigned int n_router_id = *((unsigned int *)id);
	struct alive_control_struct *control_struct = malloc(sizeof(struct alive_control_struct));
	control_struct->mutex_alive_control_struct = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;
	control_struct->num_unacked_messages = 0;
	control_struct->pid_of_control_thread = getpid();
	insert(hm_alive, n_router_id, control_struct);	
	while (1){
		//TODO loop sending messages, check num-unacked first to see if need to die
		//need to re-grab control_structure each time
		printf("alive thread proc\n");
		struct alive_control_struct *con_struct = (struct alive_control_struct*)lookup(hm_alive, n_router_id);
		pthread_mutex_lock(&con_struct->mutex_alive_control_struct);
		if (con_struct->num_unacked_messages > MAX_UNACKED_ALIVE_MESSAGES){
			//die
		}else{
			//send another message and inc count
				
			con_struct->num_unacked_messages++;
		}
		pthread_mutex_lock(&con_struct->mutex_alive_control_struct);
		usleep(10000000);

	}
}
