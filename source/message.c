/*
    Trabalho realizado pelo grupo 14
    Os elementos que compoem o grupo são:
        Bruno Liu           fc56297
        João Vedor          fc56311
        Rodrigo Cancelinha  fc56371
*/

#include "inet.h"
#include "sdmessage.pb-c.h"
#include "message-private.h"

struct message_t *create_message(){

    struct message_t *msg = (struct message_t * ) malloc(MAX_MSG); //TROCADO PELO MAX_MSG
    message_t__init(&msg->mensagem);
    if(msg == NULL){
        free(msg);
        return NULL;
    }
    return msg;
}

int read_all(int sock, uint8_t *buf, int len){
    int buffer_read = 0;
    int res;
    while(buffer_read < len) {
        res = read(sock, buf + buffer_read, len-buffer_read);
        if(res <= 0) {
            perror("Read all failed");
            return res;
        }
        buffer_read += res;
    }
    return buffer_read;
}

int write_all(int sock, uint8_t *buf, int len){
    int bufferlen = len;
    while(len > 0) {
        int res = write(sock, buf, len);
        if(res <= 0) {
            perror("Write all failed");
            return res;
        }
        buf += res;
        len -= res;
    }
    return bufferlen;
}