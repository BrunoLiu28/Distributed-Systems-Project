/*
    Trabalho realizado pelo grupo 14
    Os elementos que compoem o grupo são:
        Bruno Liu           fc56297
        João Vedor          fc56311
        Rodrigo Cancelinha  fc56371
*/

#include "client_stub-private.h"
#include "data.h"
#include "entry.h"
#include "client_stub.h"
#include "network_client.h"
#include "message-private.h"
#include "sdmessage.pb-c.h"
#include <string.h>
#include <signal.h>

/* Função para estabelecer uma associação entre o cliente e o servidor,
 * em que address_port é uma string no formato <hostname>:<port>.
 * Retorna NULL em caso de erro.
 */
struct rtree_t *rtree_connect(const char *address_port)
{

    int i = strlen(address_port);

    while ((address_port[i] != ':') && i > 0)
    {
        i--;
    }

    if (i == 0)
    {
        return NULL;
    }

    struct rtree_t *remote_tree = malloc(sizeof(struct rtree_t));
    if (remote_tree == NULL)
    {
        return NULL;
    }

    remote_tree->hostname = substr((char *)address_port, 0, i);
    remote_tree->porto = substr((char *)address_port, i + 1, strlen(address_port));

    if (network_connect(remote_tree) == -1)
    {
        free(remote_tree->hostname);
        free(remote_tree->porto);
        return NULL;
    }

    return remote_tree;
}

/* Termina a associação entre o cliente e o servidor, fechando a
 * ligação com o servidor e libertando toda a memória local.
 * Retorna 0 se tudo correr bem e -1 em caso de erro.
 */
int rtree_disconnect(struct rtree_t *rtree)
{
    struct message_t *msg = create_message();

    if (msg == NULL)
    {
        return -1;
    }

    msg->mensagem.opcode = MESSAGE_T__OPCODE__OP_ERROR;

    int nbytes;
    int tamanho_da_mensagem = message_t__get_packed_size(&(msg->mensagem));
    uint8_t *buffer = malloc(tamanho_da_mensagem);
    int tamanho_da_mensagem_aux = htonl(message_t__get_packed_size(&(msg->mensagem)));

    message_t__pack(&msg->mensagem, buffer);

    if ((nbytes = write(rtree->sockfd, &tamanho_da_mensagem_aux, sizeof(tamanho_da_mensagem))) == -1)
    {
        perror("Erro ao enviar dados ao servidor");
        close(rtree->sockfd);
        return -1;
    }

    write_all(rtree->sockfd, buffer, tamanho_da_mensagem);

    if (network_close(rtree) != 0)
    {
        return -1;
    }
    free(rtree->hostname);
    free(rtree->porto);
    free(rtree);

    return 0;
}

/* Função para adicionar um elemento na árvore.
 * Se a key já existe, vai substituir essa entrada pelos novos dados.
 * Devolve 0 (ok, em adição/substituição) ou -1 (problemas).
 */
int rtree_put(struct rtree_t *rtree, struct entry_t *entry)
{ 
    struct message_t *msg = create_message();

    if (msg == NULL)
    {
        return -1;
    }

    msg->mensagem.entry_type = (EntryT *)malloc(sizeof(EntryT));
    entry_t__init(msg->mensagem.entry_type);

    if (msg->mensagem.entry_type == NULL)
    {

        message_t__free_unpacked(&msg->mensagem, NULL);
        return -1;
    }

    msg->mensagem.entry_type->key = entry->key;

    msg->mensagem.entry_type->data = (DataT *)malloc(sizeof(DataT));
    data_t__init(msg->mensagem.entry_type->data);

    if (msg->mensagem.entry_type->data == NULL)
    {
        message_t__free_unpacked(&msg->mensagem, NULL);
        return -1;
    }

    msg->mensagem.entry_type->data->datasize = entry->value->datasize;
    msg->mensagem.entry_type->data->data = entry->value->data;

    msg->mensagem.opcode = MESSAGE_T__OPCODE__OP_PUT;
    msg->mensagem.c_type = MESSAGE_T__C_TYPE__CT_ENTRY;

    network_send_receive(rtree, msg);

    if (msg == NULL || msg->mensagem.opcode == MESSAGE_T__OPCODE__OP_ERROR)
    {
        return -1;
    }
    
    printf("O numero da sua operacao é: %d\n", msg->mensagem.op_n);
    message_t__free_unpacked(&msg->mensagem, NULL);

    return 0;
}

/* Função para obter um elemento da árvore.
 * Em caso de erro, devolve NULL.
 */
struct data_t *rtree_get(struct rtree_t *rtree, char *key)
{

    struct message_t *msg = create_message();

    if (msg == NULL)
    {
        return NULL;
    }

    msg->mensagem.entry_type = (EntryT *)malloc(sizeof(EntryT));
    entry_t__init(msg->mensagem.entry_type);

    if (msg->mensagem.entry_type == NULL)
    {
        message_t__free_unpacked(&msg->mensagem, NULL);
        return NULL;
    }

    msg->mensagem.entry_type->key = key;

    msg->mensagem.entry_type->data = (DataT *)malloc(sizeof(DataT));
    data_t__init(msg->mensagem.entry_type->data);

    if (msg->mensagem.entry_type->data == NULL)
    {
        message_t__free_unpacked(&msg->mensagem, NULL);
        return NULL;
    }

    msg->mensagem.opcode = MESSAGE_T__OPCODE__OP_GET;
    msg->mensagem.c_type = MESSAGE_T__C_TYPE__CT_KEY;

    network_send_receive(rtree, msg);

    if (msg->mensagem.entry_type->data->datasize == 0)
    {
        return NULL;
    }

    struct data_t *result = data_create2(msg->mensagem.entry_type->data->datasize, msg->mensagem.entry_type->data->data);

    return result;
}

/* Função para remover um elemento da árvore. Vai libertar
 * toda a memoria alocada na respetiva operação rtree_put().
 * Devolve: 0 (ok), -1 (key not found ou problemas).
 */
int rtree_del(struct rtree_t *rtree, char *key)
{
    struct message_t *msg = create_message();
    if (msg == NULL)
    {
        return -1;
    }

    msg->mensagem.entry_type = (EntryT *)malloc(sizeof(EntryT));
    entry_t__init(msg->mensagem.entry_type);
    if (msg->mensagem.entry_type == NULL)
    {
        return -1;
    }
    msg->mensagem.entry_type->key = key;

    msg->mensagem.opcode = MESSAGE_T__OPCODE__OP_DEL;
    msg->mensagem.c_type = MESSAGE_T__C_TYPE__CT_KEY;

    network_send_receive(rtree, msg);

    if (msg == NULL || msg->mensagem.opcode == MESSAGE_T__OPCODE__OP_ERROR)
    {
        return -1;
    }
    printf("O numero da sua operacao é: %d\n", msg->mensagem.op_n);

    return 0;
}

/* Devolve o número de elementos contidos na árvore.
 */
int rtree_size(struct rtree_t *rtree)
{
    struct message_t *msg = create_message();
    if (msg == NULL)
    {
        return -1;
    }

    msg->mensagem.opcode = MESSAGE_T__OPCODE__OP_SIZE;
    msg->mensagem.c_type = MESSAGE_T__C_TYPE__CT_NONE;

    network_send_receive(rtree, msg);

    if (msg == NULL)
    {
        return -1;
    }

    return msg->mensagem.tree_size_height;
}

/* Função que devolve a altura da árvore.
 */
int rtree_height(struct rtree_t *rtree)
{
    struct message_t *msg = create_message();
    if (msg == NULL)
    {
        return -1;
    }

    msg->mensagem.opcode = MESSAGE_T__OPCODE__OP_HEIGHT;
    msg->mensagem.c_type = MESSAGE_T__C_TYPE__CT_NONE;

    network_send_receive(rtree, msg);

    if (msg == NULL)
    {
        return -1;
    }

    return msg->mensagem.tree_size_height;
}

/* Devolve um array de char* com a cópia de todas as keys da árvore,
 * colocando um último elemento a NULL.
 */
char **rtree_get_keys(struct rtree_t *rtree)
{
    struct message_t *msg = create_message();
    if (msg == NULL)
    {
        return NULL;
    }

    msg->mensagem.opcode = MESSAGE_T__OPCODE__OP_GETKEYS;
    msg->mensagem.c_type = MESSAGE_T__C_TYPE__CT_NONE;

    network_send_receive(rtree, msg);

    if (msg->mensagem.opcode == MESSAGE_T__OPCODE__OP_ERROR)
    {
        printf("A arvore nao possui chaves\n");
        message_t__free_unpacked(&msg->mensagem, NULL);
        return NULL;
    }

    char **keys;
    int size_keys = msg->mensagem.n_keys + 1;
    keys = malloc(size_keys * sizeof(char *) + 1);
    keys = msg->mensagem.keys;

    for (int i = 0; i < size_keys - 1; i++)
    {
        keys[i] = strdup(msg->mensagem.keys[i]);
    }

    keys[size_keys - 1] = NULL;

    return keys;
}

/* Devolve um array de void* com a cópia de todas os values da árvore,
 * colocando um último elemento a NULL.
 */
void **rtree_get_values(struct rtree_t *rtree)
{

    struct message_t *msg = create_message();
    if (msg == NULL)
    {
        return NULL;
    }

    msg->mensagem.opcode = MESSAGE_T__OPCODE__OP_GETVALUES;
    msg->mensagem.c_type = MESSAGE_T__C_TYPE__CT_NONE;

    network_send_receive(rtree, msg);
    if (msg == NULL)
    {
        printf("Ocorreu um erro a realizar a operacao getvalues\n");
        message_t__free_unpacked(&msg->mensagem, NULL);
        return NULL;
    }

    void **values;
    int size_values = msg->mensagem.n_values + 1;
    values = malloc(size_values * sizeof(char *) + 1);
    values[size_values - 1] = NULL;

    return values;
}

char *substr(char *string, int initial, int final)
{
    int len = final - initial;
    char *dest = (char *)malloc(sizeof(char) * (len + 1));
    strncpy(dest, (string + initial), len);

    return dest;
}

int rtree_verify(struct rtree_t *rtree, int op_n)
{
    struct message_t *msg = create_message();
    if (msg == NULL)
    {
        return -1;
    }

    msg->mensagem.opcode = MESSAGE_T__OPCODE__OP_VERIFY;
    msg->mensagem.c_type = MESSAGE_T__C_TYPE__CT_RESULT;
    msg->mensagem.op_n = op_n;

    network_send_receive(rtree, msg);

    if (msg == NULL)
    {
        printf("Ocorreu um erro a realizar a operacao rtree_verify\n");
        message_t__free_unpacked(&msg->mensagem, NULL);
        return -1;
    }

    if (msg->mensagem.opcode == MESSAGE_T__OPCODE__OP_ERROR)
    {
        return -1;
    }

    return 0;
}