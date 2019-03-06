void *lc_thread(void *id)
{
	unsigned int n_router_id = *((unsigned int *)id);
}

void *alive_thread(void *id)
{
	unsigned int n_router_id = *((unsigned int *)id);
}

void *neighbour_thread(void *id)
{
	unsigned int n_router_id = *((unsigned int *)id);
}
