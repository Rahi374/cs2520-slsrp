#include <stdio.h>
#include <stdlib.h>

#include "hm.h"

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
	int i;
	for (i = 0; i < t->size; i++) {
		struct node *list = t->list[i];
		struct node *temp = list;
		struct node *temp2 = list;
		while (temp) {
			temp2 = temp;
			temp = temp->next;
			free(temp->val);
			free(temp);
		}
	}
	free(t->list);
	free(t);
}

void *destroyExcludeElements(struct table *t)
{
	free(t->list);
	free(t);
}

int hashCode(struct table *t, unsigned int key)
{
	if (key < 0)
		return -(key % t->size);
	return key % t->size;
}

void insert(struct table *t, unsigned int key, void *val)
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

void *lookup(struct table *t, unsigned int key)
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
	return (void*)0;
}

void *delete(struct table *t, unsigned int key)
{
	int pos = hashCode(t, key);
	struct node *list = t->list[pos];
	struct node *temp = list;
	struct node *prev = NULL;
	while (temp) {
		if (temp->key == key) {
			void *ret_val = temp->val;
			if (temp == list) {
				t->list[pos] = temp->next;
			} else {
				prev->next = temp->next;
			}
			free(temp);

			return ret_val;
		}
		prev = temp;
		temp = temp->next;
	}
	return (void*)0;
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
