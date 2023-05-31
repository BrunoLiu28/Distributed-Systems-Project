/*
    Trabalho realizado pelo grupo 14
    Os elementos que compoem o grupo são:
        Bruno Liu           fc56297
        João Vedor          fc56311
        Rodrigo Cancelinha  fc56371
*/

#include "network_server.h"
#include "tree_skel.h"
#include <stdio.h>
#include <stdlib.h>

char *sv_ip;
char *sv_port;
char *host_port;

int main(int argc, char *argv[])
{
    if(argc != 3){
        printf("Syntax do argumentos esta mal, deve ser ./tree-server <porto> <<IP>:<porta ZooKeeper>>\n");
        return -1;
    }
    printf("Servidor ligado, à espera de cliente\n");
    

    int listening_socket = network_server_init(atoi(argv[1]));
    if(listening_socket == -1){
        printf("Erro na conexao\n");
        return -1;
    }
    
    //Guardar informacoes em variaveis
    sv_port = (argv[1]);
    host_port = (argv[2]);
    char *sv_ip_aux = (char *)malloc(sizeof(argv[2]));
    strcpy(sv_ip_aux, argv[2]);
    sv_ip = strtok(sv_ip_aux, ":");

    tree_skel_init(1);
    network_main_loop(listening_socket);
    network_server_close();
    tree_skel_destroy();

    return 0;
}
