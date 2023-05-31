/*
    Trabalho realizado pelo grupo 14
    Os elementos que compoem o grupo são:
        Bruno Liu           fc56297
        João Vedor          fc56311
        Rodrigo Cancelinha  fc56371
*/

#ifndef _CLIENT_STUB_PRIVATE_H
#define _CLIENT_STUB_PRIVATE_H

#include "tree.h"
#include "inet.h"
#include "client_stub.h"

struct rtree_t{
    int sockfd;
    char *porto;
    char *hostname;
};

char* substr(char *string, int initial, int final);

#endif