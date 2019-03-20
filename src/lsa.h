#ifndef _LSA_H_
#define _LSA_H_

#include "router.h"

struct lsa_entry {
	struct full_addr neighbour_id;
	long link_cost;
};

struct lsa {
	struct full_addr router_id;
	long seq;
	int age;
	struct lsa_entry *lsa_entry_list;
	int nentries;
};

struct lsa_sending_entry {
	struct full_addr addr;
	char s;
	char a;
};

struct lsa_control_struct {
	pid_t pid_of_control_thread;
	pthread_mutex_t lock;
	struct full_addr router_id;
	struct lsa *lsa;
	struct lsa_sending_entry *lsa_sending_list;
	int nentries;
};

int lsa_is_valid(struct lsa *new_lsa, struct lsa *old_lsa);
struct lsa *extract_lsa(struct packet *packet);
struct lsa *copy_lsa(struct lsa *lsa);
void free_lsa(struct lsa *lsa);
struct lsa_sending_entry *
realloc_lsa_sending_list(struct lsa_sending_entry *lsa_sending_list, int n);

#endif // _LSA_H_
