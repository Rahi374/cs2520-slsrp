#ifndef _HASHMAP_H
#define _HASHMAP_H

#include <stdio.h>
#include <stdlib.h>

struct node {
	int key;
	int val;
	struct node *next;
};

struct table {
	int size;
	struct node **list;
};

struct table *createTable(int size)
{
	struct table *t = (struct table*)malloc(sizeof(struct table));
	t->size = size;
	t->list = (struct node**)malloc(sizeof(struct node*)*size);
	int i;
	for (i=0; i<size; i++)
		t->list[i] = NULL;
	return t;
}

void *destroyTable(struct table *t)
{
	free(t->list);
	free(t);
}

int hashCode(struct table *t,int key)
{
	if (key < 0)
		return -(key % t->size);
	return key % t->size;
}

void insert(struct table *t, int key, int val)
{
	int pos = hashCode(t, key);
	struct node *list = t->list[pos];
	struct node *newNode = (struct node*)malloc(sizeof(struct node));
	struct node *temp = list;
	while (temp) {
		if (temp->key == key) {
			temp->val = val;
			return;
		}
		temp = temp->next;
	}
	newNode->key = key;
	newNode->val = val;
	newNode->next = list;
	t->list[pos] = newNode;
}

int lookup(struct table *t, int key)
{
	int pos = hashCode(t, key);
	struct node *list = t->list[pos];
	struct node *temp = list;
	while (temp) {
		if (temp->key == key) {
			return temp->val;
		}
		temp = temp->next;
	}
	return -1;
}

int delete(struct table *t, int key)
{
	int pos = hashCode(t, key);
	struct node *list = t->list[pos];
	struct node *temp = list;
	struct node *prev = NULL;
	while (temp) {
		if (temp->key == key) {
			//removing the first node
			if (temp == list) {
				t->list[pos] = temp->next;
			} else {
				prev->next = temp->next;
			}
			free(temp);

			return 1;
		}
		prev = temp;
		temp = temp->next;
	}
	return -1;
}

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
