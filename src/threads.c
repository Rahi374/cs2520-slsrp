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
	while (1){
		printf("alive thread proc\n");
		struct alive_control_struct *con_struct = (struct alive_control_struct*)lookup(hm_alive, n_router_id);
		pthread_mutex_lock(&con_struct->mutex_alive_control_struct);
		if (con_struct->num_unacked_messages > MAX_UNACKED_ALIVE_MESSAGES){
			//die
			//TODO remove the neighbor
			free(id);
			free(con_struct);
		}else{
			//send another message and inc count
			con_struct->num_unacked_messages++;
		}
		pthread_mutex_lock(&con_struct->mutex_alive_control_struct);
		usleep(1000000);

	}
}
