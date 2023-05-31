/*
    Trabalho realizado pelo grupo 14
    Os elementos que compoem o grupo são:
        Bruno Liu           fc56297
        João Vedor          fc56311
        Rodrigo Cancelinha  fc56371
*/

#ifndef _TREE_SKEL_PRIVATE_H
#define _TREE_SKEL_PRIVATE_H

#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdlib.h>
#include "message-private.h"

struct pointers
{
	int in;
	int out;
};

/*Estrutura para a lista
*/
struct circular_list
{
	struct pointers *ptrs;
	struct request_t *buffer;
};

/*Aloca memoria para a lista
*/
void *create_shared_memory(char *name, int size);

/*Apaga memoria para a lista
*/
void destroy_shared_memory(char *name, void *ptr, int size);

/*Criação da lista
*/
struct circular_list *create_list();

/*Destruição da lista
*/
void destroy_list(struct circular_list *list);

/*Adiciona elemento à lista
*/
void write_list(struct circular_list *list, struct request_t *request);

/*Lê elemento da lista e remove-o
*/
struct request_t *read_list(struct circular_list *list);

/*Verifica se a lista está vazia
*/
int is_list_empty(struct circular_list *list);

#endif