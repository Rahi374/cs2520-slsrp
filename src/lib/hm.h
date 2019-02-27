#ifndef _HASHMAP_H
#define _HASHMAP_H

struct node {
	int key;
	int val;
	struct node *next;
};

struct table {
	int size;
	struct node **list;
};

struct table *createTable(int size);
void *destroyTable(struct table *t);
int hashCode(struct table *t,int key);
void insert(struct table *t, int key, int val);
int lookup(struct table *t, int key);
int delete(struct table *t, int key);

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
