/*
    Trabalho realizado pelo grupo 14
    Os elementos que compoem o grupo são:
        Bruno Liu           fc56297
        João Vedor          fc56311
        Rodrigo Cancelinha  fc56371
*/

#include "client_stub.h"
#include "client_stub-private.h"
#include "sdmessage.pb-c.h"
#include "network_client.h"
#include "inet.h"
#include "message-private.h"
#include <signal.h>


/* Esta função deve:
 * - Obter o endereço do servidor (struct sockaddr_in) a base da
 *   informação guardada na estrutura rtree;
 * - Estabelecer a ligação com o servidor;
 * - Guardar toda a informação necessária (e.g., descritor do socket)
 *   na estrutura rtree;
 * - Retornar 0 (OK) ou -1 (erro).
 */
int network_connect(struct rtree_t *rtree){

    if ((rtree->sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Erro ao criar socket TCP");
        return -1;
    }

    struct sockaddr_in server;
    server.sin_family = AF_INET; 
    server.sin_port = htons(atoi(rtree->porto)); 
    server.sin_addr.s_addr = htonl(INADDR_ANY);


    if (connect(rtree->sockfd,(struct sockaddr *)&(server), sizeof(server)) < 0) {
        perror("Erro ao conectar-se ao servidor");
        close(rtree->sockfd);
        return -1;
    }

    return 0;
}

/* Esta função deve:
 * - Obter o descritor da ligação (socket) da estrutura rtree_t;
 * - Serializar a mensagem contida em msg;
 * - Enviar a mensagem serializada para o servidor;
 * - Esperar a resposta do servidor;
 * - De-serializar a mensagem de resposta;
 * - Retornar a mensagem de-serializada ou NULL em caso de erro.
 */
struct message_t *network_send_receive(struct rtree_t * rtree, struct message_t *msg){

    int nbytes;
    int tamanho_da_mensagem = message_t__get_packed_size(&(msg->mensagem));
    uint8_t *buffer = malloc(tamanho_da_mensagem);
    int tamanho_da_mensagem_aux = htonl(message_t__get_packed_size(&(msg->mensagem)));
    message_t__pack(&msg->mensagem,buffer);
    
    if((nbytes = write(rtree->sockfd, &tamanho_da_mensagem_aux,sizeof(tamanho_da_mensagem))) == -1){
        perror("Erro ao enviar dados ao servidor");
        close(rtree->sockfd);
        return NULL;
    }
    write_all(rtree->sockfd, buffer, tamanho_da_mensagem);

    //----------------------------------------------------------------------

    int tamanho_resposta;
    (nbytes = read(rtree->sockfd,&tamanho_resposta,sizeof(tamanho_resposta)));

    if((nbytes) == -1){
        perror("Erro ao receber dados do servidor");
        close(rtree->sockfd);
        return NULL;
    };

    int tamanho_a_ler = ntohl(tamanho_resposta);
    uint8_t mensagem[tamanho_a_ler];
    read_all(rtree->sockfd,mensagem, tamanho_a_ler);
    mensagem[tamanho_a_ler] = '\0';
    MessageT *aux = (message_t__unpack(NULL,tamanho_a_ler, mensagem));
    msg->mensagem = *(message_t__unpack(NULL,tamanho_a_ler, mensagem));
    if(aux->opcode == MESSAGE_T__OPCODE__OP_ERROR){
        return NULL;
    }

    return msg;
}

/* A função network_close() fecha a ligação estabelecida por
 * network_connect().
 */
int network_close(struct rtree_t * rtree){
    return close(rtree->sockfd);
}

