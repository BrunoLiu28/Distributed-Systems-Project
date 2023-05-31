/*
    Trabalho realizado pelo grupo 14
    Os elementos que compoem o grupo são:
        Bruno Liu           fc56297
        João Vedor          fc56311
        Rodrigo Cancelinha  fc56371
*/

#include "inet.h"
#include "message-private.h"
#include "tree_skel.h"
#include "network_server.h"
#include <signal.h>
#include <poll.h>

#define NFDESC 10 // N�mero de sockets (uma para listening)

void network_server_close_aux(int signum);

int sockfd;
struct sockaddr_in server, client;
/* Função para preparar uma socket de receção de pedidos de ligação
 * num determinado porto.
 * Retornar descritor do socket (OK) ou -1 (erro).
 */
int network_server_init(short port){

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        perror("Erro ao criar socket TCP");
        return -1;
    }

    int stay_port = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR,&stay_port,sizeof(int));

    server.sin_family = AF_INET;
    server.sin_port = htons(port); 
    server.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(sockfd, (struct sockaddr *)&server, sizeof(server)) < 0)
    {
        perror("Erro ao fazer bind");
        close(sockfd);
        return -1;
    }
    
    if (listen(sockfd, 0) < 0)
    {
        perror("Erro ao executar listen");
        close(sockfd);
        return -1;
    }

    return sockfd;
}

/* Esta função deve:
 * - Aceitar uma conexão de um cliente;
 * - Receber uma mensagem usando a função network_receive;
 * - Entregar a mensagem de-serializada ao skeleton para ser processada;
 * - Esperar a resposta do skeleton;
 * - Enviar a resposta ao cliente usando a função network_send.
 */
int network_main_loop(int listening_socket)
{
    struct pollfd connections[NFDESC];
    for (int i = 0; i < NFDESC; i++){
        connections[i].fd = -1;    // poll ignora estruturas com fd < 0
    }
    connections[0].fd = listening_socket;  // Vamos detetar eventos na welcoming socket
    connections[0].events = POLLIN;
    int nfds = 1; // n�mero de file descriptors

    socklen_t size_client;
    int kfds;
    size_client = sizeof(client);
    signal(SIGINT,network_server_close_aux);
    while ((kfds = poll(connections, nfds, 10)) >= 0){ // kfds == 0 significa timeout sem eventos
        
        if (kfds > 0){ // kfds � o n�mero de descritores com evento ou erro

            if ((connections[0].revents & POLLIN) && (nfds < NFDESC)) 
                if ((connections[nfds].fd = accept(connections[0].fd, (struct sockaddr *) &client, &size_client)) > 0){ // Liga��o feita ?
                    connections[nfds].events = POLLIN; // Vamos esperar dados nesta socket
                    nfds++;
                    printf("Cliente connectado\n");
                }
            for (int i = 1; i < nfds; i++){ // Todas as liga��es
            
                if (connections[i].revents & POLLIN) { // Dados para ler
                    struct message_t *message = network_receive(connections[i].fd);
                    
                    if (message == NULL || message->mensagem.opcode == MESSAGE_T__OPCODE__OP_ERROR)
                    {
                        free(message);
                        printf("Cliente Disconectado!\n");
                        close(connections[i].fd);
                        connections[i].fd = -1;
                        continue;
                    }
                    invoke(message);
                    int value_send = network_send(connections[i].fd, message);
                    if (value_send == -1)
                    {
                        close(connections[i].fd);
                        connections[i].fd = -1;
                        continue;
                    }
                    
                }
            }
        }
    }
    
    close(listening_socket);
    return 0;
}

/* Esta função deve:
 * - Ler os bytes da rede, a partir do client_socket indicado;
 * - De-serializar estes bytes e construir a mensagem com o pedido,
 *   reservando a memória necessária para a estrutura message_t.
 */
struct message_t *network_receive(int client_socket){
    
    struct message_t *msg = create_message();
    int value;
    int tamanho_resposta;

    value = read(client_socket, &tamanho_resposta, sizeof(int));

    if ((value) < 0){
        free(msg);
        perror("Erro ao receber dados do servidor");
        close(client_socket);
        return NULL;
    } 

    int tamanho_a_ler = ntohl(tamanho_resposta);
    uint8_t mensagem[tamanho_a_ler];
    read_all(client_socket, mensagem, tamanho_a_ler);
    mensagem[tamanho_a_ler] = '\0';
    MessageT *aux = (message_t__unpack(NULL, tamanho_a_ler, mensagem));
    msg->mensagem = *aux;

    return msg;
}

/* Esta função deve:
 * - Serializar a mensagem de resposta contida em msg;
 * - Libertar a memória ocupada por esta mensagem;
 * - Enviar a mensagem serializada, através do client_socket.
 */
int network_send(int client_socket, struct message_t *msg){
    int tamanho_da_mensagem = (message_t__get_packed_size(&(msg->mensagem)));
    uint8_t *buffer = malloc(tamanho_da_mensagem);
    int tamanho_da_mensagem_aux = htonl(tamanho_da_mensagem);
    message_t__pack(&msg->mensagem,buffer);

    if ((write(client_socket, &tamanho_da_mensagem_aux, sizeof(int))) == -1)
    {
        perror("Erro ao enviar dados ao servidor");
        close(client_socket);
        return -1;
    }

    write_all(client_socket, buffer, tamanho_da_mensagem);
    
    return 0;
}

/* A função network_server_close() liberta os recursos alocados por
 * network_server_init().
 */
int network_server_close(){
    printf("\nA desligar o servidor\n");
    tree_skel_destroy();
    close(sockfd);
    return 0;
}

void network_server_close_aux(int signum){
  network_server_close();
  exit(1);
}

