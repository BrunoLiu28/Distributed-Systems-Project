/*
    Trabalho realizado pelo grupo 14
    Os elementos que compoem o grupo são:
        Bruno Liu           fc56297
        João Vedor          fc56311
        Rodrigo Cancelinha  fc56371
*/

/*
    Trabalho realizado pelo grupo 14
    Os elementos que compoem o grupo são:
        Bruno Liu           fc56297
        João Vedor          fc56311
        Rodrigo Cancelinha  fc56371
*/

#ifndef _MESSAGE_PRIVATE_H
#define _MESSAGE_PRIVATE_H

#include "inet.h"
#include "sdmessage.pb-c.h"

struct message_t{
    MessageT mensagem;
};

struct request_t {
    int op_n;                           //o número da operação
    int op;                             //a operação a executar. op=0 se for um delete, op=1 se for um put
    char* key;                          //a chave a remover ou adicionar
    struct data_t *data;                // os dados a adicionar em caso de put, ou NULL em caso de delete
};

struct message_t *create_message();

int read_all(int sock, uint8_t *buf, int len);
int write_all(int sock, uint8_t *buf, int len);

#endif