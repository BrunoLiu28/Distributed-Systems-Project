/*
    Trabalho realizado pelo grupo 14
    Os elementos que compoem o grupo são:
        Bruno Liu           fc56297
        João Vedor          fc56311
        Rodrigo Cancelinha  fc56371
*/

#ifndef _TREE_PRIVATE_H
#define _TREE_PRIVATE_H

#include "tree.h"
#include "entry.h"

struct tree_t {
	struct node *root;
	int size;
};

struct node {
	struct entry_t *entry;
	struct node *left;
	struct node *right;
};

void tree_destroy_aux(struct node *node);

int tree_put_aux(struct node **node, struct entry_t *new_entry);

struct data_t *tree_get_aux(struct node **node, char *key);

int key_compare(char *key, struct entry_t *entry2);

int tree_del_aux(struct node **node, char *key);

struct node *remove_largest_child(struct node **node);

int tree_height_aux(struct node *node);

int tree_get_keys_aux(char **chaves, struct node *node, int counter);

int tree_get_values_aux(void **valores, struct node *node, int counter);

int max(int num1, int num2);

#endif
