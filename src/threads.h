#ifndef _THREADS_H_
#define _THREADS_H_

void *lc_thread(void *id);
void *alive_thread(void *id);
void *add_neighbour_thread(void *id);
void *lsa_sending_thread(void *id);
void *lsa_generating_thread(void *id);

#endif // _THREADS_H_
