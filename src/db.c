#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "lib/db.h"
#include "hm.h"
#include "lsa.h"
#include "router.h"
#include "tools.h"

int minDistance(long dist[], int *sptSet, int n) 
{  
	long min = LONG_MAX;
	int min_index; 
	int v;
	for (v = 0; v < n; v++) 
	if (sptSet[v] == 0 && dist[v] <= min) 
	    min = dist[v], min_index = v; 

	return min_index; 
}

void dijkstra(int n, int *next, void *cm, int src) 
{ 
	long graph[n][n];
	memcpy(graph, cm, (sizeof(long)*n*n));
	long dist[n];
	int sptSet[n];  

	int parent[n];

	int i;
	for (i = 0; i < n; i++) {
	parent[0] = -1; 
	dist[i] = LONG_MAX; 
	sptSet[i] = 0; 
	}

	dist[src] = 0;

	int count;
	for (count = 0; count < n - 1; count++) {
		int u = minDistance(dist, sptSet, n);
		sptSet[u] = 1;
		int v;
		for (v = 0; v < n; v++) {
			if (sptSet[v] == 0 && graph[u][v] && dist[u] + graph[u][v] < dist[v] && graph[u][v] != LONG_MAX) {
				parent[v] = u;
				dist[v] = dist[u] + graph[u][v];
			}
		}
	}
	memcpy(next, &parent, sizeof(int)*n);
}

void update_rt()
{
	dprintf("starting rt update\n");
	struct rt_entry *new_rt;
	struct table *new_hm_rt_index;
	//lock hm_lsa
	
	pthread_mutex_lock(&mutex_hm_lsa);

	//get num nodes
	int n = lsa_count;

	if (n <= 1) {//if only one then there is only us so no need for a rt
		pthread_mutex_unlock(&mutex_hm_lsa);	
		dprintf("no record of nodes for rt table\n");
		return;
	}
	struct rt_entry rt_arr[n];

	//make space for routing table
	new_rt = (struct rt_entry*)malloc(sizeof(struct rt_entry)*n);

	int port_nums[n];//store the ports for the ind's
	unsigned int all_addrs[n];

	//create 2d array for adj matrix
	long cm[n][n];
	int i, j;
	for (i = 0; i < n; i++) {
		for (j = 0; j < n; j++) {
			if (i == j) {
				cm[i][j] = 0;
			} else {
				cm[i][j] = LONG_MAX;
			}
		}
	}
	
	new_hm_rt_index = createTable(100);
	
	//fill 2d array based on lsa links using associated index
	//loop through all lsas:
	//	if id not in hm
	//		add (id,index) to hm
	//	loop through all entries in lsa:
	//		if neighbor not in hm
	//			add neighbor to hm
	//		add link cost to cm[ind1][ind2] and cm[ind2][ind1] //or maybe let other neighbor add other
	int router_num = 1;
	for (i = 0; i < hm_lsa->size; i++) {
		struct node *list = hm_lsa->list[i];	
		struct node *temp = list;
		while (temp) {
			struct lsa_control_struct *lsa_con;
			lsa_con = (struct lsa_control_struct*)temp->val;
			int r_id_ind = (intptr_t)lookup(new_hm_rt_index, lsa_con->router_id.addr.s_addr);
			if (r_id_ind == 0) {
				insert(new_hm_rt_index, lsa_con->router_id.addr.s_addr, (void*)(intptr_t)router_num);
				r_id_ind = router_num;
				router_num++;
			}
			r_id_ind--;//okay so gross but we start at 1 and sub 1 cause hm grossness
			all_addrs[r_id_ind] = lsa_con->router_id.addr.s_addr; 
			port_nums[r_id_ind] = lsa_con->router_id.port;//save for use in rt
			int l;
			for (l = 0; l < lsa_con->lsa->nentries; l++) {
				int n_id_ind = (intptr_t)lookup(new_hm_rt_index, lsa_con->lsa->lsa_entry_list[l].neighbour_id.addr.s_addr);
				if (n_id_ind == 0) {
					insert(new_hm_rt_index, lsa_con->lsa->lsa_entry_list[l].neighbour_id.addr.s_addr, (void*)(intptr_t)router_num);
					n_id_ind = router_num;
					router_num++;
				}
				n_id_ind--;//okay so gross but we start at 1 and sub 1 cause hm grossness
				cm[r_id_ind][n_id_ind] = lsa_con->lsa->lsa_entry_list[l].link_cost;
				cm[n_id_ind][r_id_ind] = lsa_con->lsa->lsa_entry_list[l].link_cost;
			}
			struct rt_entry rt_ent;
			memset(&rt_ent, 0, sizeof(struct rt_entry));
			rt_ent.to_addr.s_addr = lsa_con->router_id.addr.s_addr;
			//dprintf("***router: %s, has index: %d\n", inet_ntoa(lsa_con->router_id.addr), r_id_ind);
			memcpy(&rt_arr[r_id_ind], &rt_ent, sizeof(struct rt_entry)); 
			temp = temp->next;
		}
	}
	/*
	printf("print cost matrix\n");
	int h, c;
	for(h = 0; h < n; h++){
		for(c = 0; c < n; c++){
			printf(" %ld", cm[h][c]);	
		}
		printf("\n");
	}
	*/
	long dis[n];
	int next[n];
	int start = (intptr_t)lookup(new_hm_rt_index, cur_router_id.s_addr);
	if (start == 0) {
		printf("\n\nError: lookup was 0 in db start index lookup\n\n");
		return;
	}
	start--;//okay so gross but we start at 1 and sub 1 cause hm grossness
	dijkstra(n, next, (void*)cm, start);

	next[start] = start;
	/*
	printf("\nnext contains:");
	int k;
	for (k = 0; k < n; k++) {
		printf(" %d", next[k]);
	}
	printf("\n");
	fflush(stdout);
	*/
	//walk next back for each of the nodes
	for(i = 0; i < n; i++){
		if (all_addrs[i] != cur_router_id.s_addr) {
			int temp_ind = i;
			while (next[temp_ind] != start) {
				temp_ind = next[temp_ind];
			}
			//temp_ind is the index of the router to send to
			rt_arr[i].thru_addr.s_addr = all_addrs[temp_ind];
			rt_arr[i].thru_port = port_nums[temp_ind];
		}
	}

	memcpy(new_rt, &rt_arr, sizeof(struct rt_entry)*n);

	//if old rt is not 0, free the mem after swapping pointer
	if (rt == 0) {
		rt = new_rt;
		hm_rt_index = new_hm_rt_index;
		pthread_mutex_unlock(&mutex_hm_lsa);
	}else{
		void *tmp_rt = rt;
		rt = new_rt;
		struct table *tmp_hm_rt_index = hm_rt_index;
		hm_rt_index = new_hm_rt_index;

		pthread_mutex_unlock(&mutex_hm_lsa);
		usleep(1000000);
		destroyExcludeElements(tmp_hm_rt_index);
		free(tmp_rt);

	}
}

