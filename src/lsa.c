#include <stdlib.h>
#include <string.h>

#include "lsa.h"
#include "router.h"

int lsa_is_valid(struct lsa *new_lsa, struct lsa *old_lsa)
{
	if (new_lsa->seq > old_lsa->seq)
		return 1;

	if (--old_lsa->age <= 0)
		return 1;

	return 0;
}

struct lsa *extract_lsa(struct packet *packet)
{
	struct lsa *lsa = packet->data;
	int i;
	int count = 0;

	// i hope this works
	lsa->lsa_entry_list = (struct lsa_entry *)(((char *)packet->data) + sizeof(struct lsa));

	return lsa;
}

struct lsa *copy_lsa(struct lsa *lsa)
{
	struct lsa *lsa_copy = malloc(sizeof(struct lsa));
	memcpy(lsa_copy, lsa, sizeof(struct lsa));

	struct lsa_entry *lsa_entries = calloc(lsa->nentries, sizeof(struct lsa_entry));
	lsa_copy->lsa_entry_list = lsa_entries;
	memcpy(lsa_copy->lsa_entry_list, lsa->lsa_entry_list,
	       lsa->nentries * sizeof(struct lsa_entry));

	return lsa_copy;
}

void free_lsa(struct lsa *lsa)
{
	free(lsa->lsa_entry_list);
	free(lsa);
}

struct lsa_sending_entry *
realloc_lsa_sending_list(struct lsa_sending_entry *lsa_sending_list, int n)
{
	lsa_sending_list = realloc(lsa_sending_list,
				   n * sizeof(struct lsa_sending_entry));
	memset(lsa_sending_list, 0, n * sizeof(struct lsa_sending_entry));

	return lsa_sending_list;
}
