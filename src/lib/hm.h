#ifndef _HASHMAP_H
#define _HASHMAP_H

struct node {
	unsigned int key;
	void *val;
	struct node *next;
};

struct table {
	int size;
	struct node **list;
};

struct table *createTable(int size);
void *destroyTable(struct table *t);
void *destroyExcludeElements(struct table *t);
int hashCode(struct table *t, unsigned int key);
void insert(struct table *t, unsigned int key, void *val);
void* lookup(struct table *t, unsigned int key);
void* delete(struct table *t, unsigned int key);

/*
   int main()
   {
   struct table *t = createTable(5);
   insert(t,2,3);
   insert(t,5,4);
   printf("%d\n",lookup(t,5));
   printf("%d\n",lookup(t,2));
   delete(t,5);
   printf("%d\n",lookup(t,5));
   insert(t,5,4);
   printf("%d\n",lookup(t,5));
   delete(t,5);
   printf("%d\n",lookup(t,5));
   insert(t,5,4);
   printf("%d\n",lookup(t,5));
   return 0;
   }
   */

#endif // _HASHMAP_H
