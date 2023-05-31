/*
    Trabalho realizado pelo grupo 14
    Os elementos que compoem o grupo são:
        Bruno Liu           fc56297
        João Vedor          fc56311
        Rodrigo Cancelinha  fc56371
*/

#include "message-private.h"
#include "sdmessage.pb-c.h"
#include "tree.h"
#include "tree_skel.h"
#include "tree_skel-private.h"
#include "network_server.h"
#include "client_stub.h"
#include "client_stub-private.h"
#include <errno.h>
#include <pthread.h>
#include <zookeeper/zookeeper.h>

#define list_size 100 // tamanho por default

typedef struct String_vector zoo_string;

struct op_proc
{
    int max_proc;
    int *in_progress;
};

struct tree_t *tree;
struct op_proc *proc;
struct request_t *queue_head;
struct circular_list *lista;
int number_threads;
int last_assigned;

//---------------
extern char *sv_ip;
extern char *sv_port;
extern char *host_port;

static zhandle_t *zh;
struct rtree_t *next_server;
const char *my_zoo_id;
static int is_connected;
static char *root_path = "/chain";
#define SECTHREAD 1
static char *watcher_ctx = "ZooKeeper Data Watcher";
char *new_path;
//---------------

pthread_mutex_t fila, arvore = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t fila_condition = PTHREAD_COND_INITIALIZER;

void connection_watcher(zhandle_t *zzh, int type, int state, const char *path, void *context)
{
    if (type == ZOO_SESSION_EVENT)
    {
        if (state == ZOO_CONNECTED_STATE)
        {
            is_connected = 1;
        }
        else
        {
            is_connected = 0;
        }
    }
}

static void child_watcher(zhandle_t *wzh, int type, int state, const char *zpath, void *watcher_ctx)
{
    zoo_string *children_list = (zoo_string *)malloc(sizeof(zoo_string));
    if (state == ZOO_CONNECTED_STATE)
    {
        if (type == ZOO_CHILD_EVENT)
        {
            if (ZOK != zoo_wget_children(zh, root_path, child_watcher, watcher_ctx, children_list))
            {
                fprintf(stderr, "Error setting watch at %s!\n", root_path);
            }

            char *highest_than_ours;

            for (int i = 0; i < children_list->count; i++)
            {
                if (i == 0)
                {
                    highest_than_ours = children_list->data[i];
                }
                else if (strcmp(my_zoo_id, children_list->data[i]) < 0)
                {
                    if (strcmp(highest_than_ours, my_zoo_id) > 0)
                    {
                        if (strcmp(highest_than_ours, children_list->data[i]) > 0)
                        {
                            highest_than_ours = children_list->data[i];
                        }
                    }
                    else
                    {
                        highest_than_ours = children_list->data[i];
                    }
                }

            }

            if (strcmp(my_zoo_id, highest_than_ours) >= 0)
            {
                next_server = NULL;
            }
            else
            {
                char path[120] = "";
                strcat(path, root_path);
                strcat(path, "/");
                strcat(path, highest_than_ours); // PATH EXEMPLO /chain/node0000000030
                int highest_length = 128;
                char highest_buffer[highest_length];
                zoo_get(zh, path, 0, highest_buffer, &highest_length, NULL);

                // CONNECT NEXT SERVER
                char next_aux[128];
                strcpy(next_aux, sv_ip);
                strcat(next_aux, ":");
                strcat(next_aux, highest_buffer);

                sleep(2);
                next_server = rtree_connect(next_aux);
            }
        }
    }
    free(children_list);
}

/* Inicia o skeleton da árvore.
 * O main() do servidor deve chamar esta função antes de poder usar a
 * função invoke().
 * Retorna 0 (OK) ou -1 (erro, por exemplo OUT OF MEMORY)
 */
int tree_skel_init(int N)
{
    zoo_string *children_list = NULL;

    zh = zookeeper_init(host_port, connection_watcher, 2000, 0, 0, 0);

    if (zh == NULL)
    {
        fprintf(stderr, "Error connecting to ZooKeeper server[%d]!\n", errno);
        exit(EXIT_FAILURE);
    }

    //Se o /chain nao existe, cria
    int buf_len = 128;
    char buf[128];
    if (ZNONODE == zoo_exists(zh, root_path, 0, NULL))
    {
        zoo_create(zh, root_path, "hello", 10, &ZOO_OPEN_ACL_UNSAFE, 0, buf, buf_len);
    }

    char node_path[120] = "";
    int new_path_len = 1024;
    new_path = malloc(new_path_len);
    strcat(node_path, "/chain");
    strcat(node_path, "/node");

    //  Criar um ZNode efémero
    if (ZOK != zoo_create(zh, node_path, sv_port, 20, &ZOO_OPEN_ACL_UNSAFE, ZOO_EPHEMERAL | ZOO_SEQUENCE, new_path, new_path_len))
    {
        fprintf(stderr, "Error creating znode from path %s!\n", node_path);
        exit(EXIT_FAILURE);
    }


    // DEFINIR MY ID
    char *path_aux = (char *)malloc(sizeof(new_path));
    strcpy(path_aux, new_path);
    strtok(path_aux, "/");
    my_zoo_id = strtok(NULL, "/");

    //• Obter e fazer watch aos filhos de /chain;
    children_list = (zoo_string *)malloc(sizeof(zoo_string));
    if (ZOK != zoo_wget_children(zh, root_path, &child_watcher, watcher_ctx, children_list))
    {
        fprintf(stderr, "Error setting watch at %s!\n", root_path);
    }

    //• Ver qual é o servidor com id mais alto a seguir ao nosso, de entre os filhos de /chain;
    char *highest_than_ours = NULL;
    for (int i = 0; i < children_list->count; i++)
    {

        if (i == 0)
        {
            highest_than_ours = children_list->data[i];
        }
        else if (strcmp(my_zoo_id, children_list->data[i]) < 0)
        {
            if (strcmp(highest_than_ours, my_zoo_id) > 0)
            {
                if (strcmp(highest_than_ours, children_list->data[i]) > 0)
                {
                    highest_than_ours = children_list->data[i];
                }
            }
            else
            {
                highest_than_ours = children_list->data[i];
            }
        }

    }

    // DEFINIR NEXT SERVER
    if (strcmp(my_zoo_id, highest_than_ours) >= 0)
    {
        next_server = NULL;
    }
    else
    {
        char path[120] = "";
        strcat(path, root_path);
        strcat(path, "/");
        strcat(path, highest_than_ours); // PATH EXEMPLO /chain/node0000000030
        int highest_length = 128;
        char highest_buffer[highest_length];
        zoo_get(zh, path, 0, highest_buffer, &highest_length, NULL);

        // CONNECT NEXT SERVER
        char next_aux[128];
        strcpy(next_aux, sv_ip);
        strcat(next_aux, ":");
        strcat(next_aux, highest_buffer);
        
        sleep(2);
        next_server = rtree_connect(next_aux);
    }

    //------------------------------------------
    tree = tree_create();
    if (tree == NULL)
    {
        return -1;
    }
    last_assigned = 0;

    number_threads = SECTHREAD;
    proc = malloc(sizeof(proc));
    proc->max_proc = 0;
    proc->in_progress = malloc(SECTHREAD * sizeof(int));
    for (int i = 0; i < number_threads; i++)
    {
        proc->in_progress[i] = 0;
    }

    lista = create_list();

    pthread_t threads[SECTHREAD];
    int thread_param[SECTHREAD];
    for (int i = 0; i < SECTHREAD; i++)
    {
        thread_param[i] = i + 1;
        if (pthread_create(&threads[i], NULL, &process_request, (void *)&thread_param[i]) != 0)
        {
            printf("\nThread %d não criada.\n", i);
            exit(EXIT_FAILURE);
        }
    }

    return 0;
}

/* Função da thread secundária que vai processar pedidos de escrita.
 */
void *process_request(void *params)
{
    while (1)
    {

        pthread_mutex_lock(&fila);

        while (is_list_empty(lista))
        { // Ver se lista esta vazia
            pthread_cond_wait(&fila_condition, &fila);
        }
        queue_head = read_list(lista);
        printf("A tratar do pedido - %d\n", queue_head->op_n);
        pthread_cond_broadcast(&fila_condition);
        pthread_mutex_unlock(&fila);

        for (int i = 0; i < number_threads; i++)
        {
            if (proc->in_progress[i] == 0)
            {
                proc->in_progress[i] = queue_head->op_n;
                i = number_threads;
            }
        }

        if (queue_head->op == 0)
        { // delete
            pthread_mutex_lock(&arvore);
            tree_del(tree, queue_head->key);
            if (proc->max_proc < queue_head->op_n)
            {
                proc->max_proc = queue_head->op_n;
            }
            for (int i = 0; i < number_threads; i++)
            {
                if (proc->in_progress[i] == queue_head->op_n)
                {
                    proc->in_progress[i] = 0;
                    i = number_threads;
                }
            }
            pthread_cond_broadcast(&fila_condition);
            pthread_mutex_unlock(&arvore);

            // mandar para o proximo servidor ate chegar à cauda
            if (next_server != NULL)
            {
                rtree_del(next_server, queue_head->key);
                while(rtree_verify(next_server, last_assigned) == -1){
                    rtree_del(next_server, queue_head->key);
                }
            }
        }
        else if (queue_head->op == 1)
        { // put
            pthread_mutex_lock(&arvore);
            tree_put(tree, queue_head->key, queue_head->data);
            if (proc->max_proc < queue_head->op_n)
            {
                proc->max_proc = queue_head->op_n;
            }
            for (int i = 0; i < number_threads; i++)
            {
                if (proc->in_progress[i] == queue_head->op_n)
                {
                    proc->in_progress[i] = 0;
                    i = number_threads;
                }
            }
            pthread_cond_broadcast(&fila_condition);
            pthread_mutex_unlock(&arvore);

            // mandar para o proximo servidor ate chegar à cauda
            if (next_server != NULL)
            {
                struct entry_t *next_aux = entry_create(queue_head->key, queue_head->data);
                rtree_put(next_server, next_aux);
                while(rtree_verify(next_server, last_assigned) == -1){
                    rtree_put(next_server, next_aux);
                }
                
            }
        }

    }
}

/* Liberta toda a memória e recursos alocados pela função tree_skel_init.
 */
void tree_skel_destroy()
{
    if (next_server != NULL){
        rtree_disconnect(next_server);
    }

    char path[120] = "";
    strcat(path, root_path);
    strcat(path, "/");
    strcat(path, my_zoo_id);
    zoo_delete (zh, path, -1);

    free(proc->in_progress);
    free(proc);
    destroy_list(lista);
    tree_destroy(tree);
}

/* Executa uma operação na árvore (indicada pelo opcode contido em msg)
 * e utiliza a mesma estrutura message_t para devolver o resultado.
 * Retorna 0 (OK) ou -1 (erro, por exemplo, árvore nao incializada)
 */
int invoke(struct message_t *msg)
{
    if (tree == NULL)
    {
        return -1;
    }

    int size;
    char *key;
    struct request_t *aux;
    switch (msg->mensagem.opcode)
    {
    case MESSAGE_T__OPCODE__OP_BAD:
    {
        break;
    }
    case MESSAGE_T__OPCODE__OP_SIZE:
    { // SIZE
        pthread_mutex_lock(&arvore);
        size = tree_size(tree);
        if (size < 0)
        {
            msg->mensagem.opcode = MESSAGE_T__OPCODE__OP_ERROR;
            msg->mensagem.c_type = MESSAGE_T__C_TYPE__CT_NONE;
            pthread_mutex_unlock(&arvore);
            return -1;
        }
        msg->mensagem.opcode = MESSAGE_T__OPCODE__OP_SIZE + 1;
        msg->mensagem.c_type = MESSAGE_T__C_TYPE__CT_RESULT;
        msg->mensagem.tree_size_height = size;
        pthread_mutex_unlock(&arvore);
        return 0;
        break;
    }
    case MESSAGE_T__OPCODE__OP_HEIGHT:
    { // HEIGHT
        pthread_mutex_lock(&arvore);
        size = tree_height(tree);
        if (size < 0)
        {
            msg->mensagem.opcode = MESSAGE_T__OPCODE__OP_ERROR;
            msg->mensagem.c_type = MESSAGE_T__C_TYPE__CT_NONE;
            pthread_mutex_unlock(&arvore);
            return -1;
        }
        msg->mensagem.opcode = MESSAGE_T__OPCODE__OP_HEIGHT + 1;
        msg->mensagem.c_type = MESSAGE_T__C_TYPE__CT_RESULT;
        msg->mensagem.tree_size_height = size;
        pthread_mutex_unlock(&arvore);
        return 0;
        break;
    }
    case MESSAGE_T__OPCODE__OP_DEL:
    { // DEL

        last_assigned++;
        aux = (struct request_t *)malloc(sizeof(struct request_t));
        aux->data = NULL;
        aux->key = msg->mensagem.entry_type->key;
        aux->op_n = last_assigned;
        aux->op = 0;
        write_list(lista, aux);
        pthread_cond_broadcast(&fila_condition);
        msg->mensagem.opcode = MESSAGE_T__OPCODE__OP_DEL + 1;
        msg->mensagem.c_type = MESSAGE_T__C_TYPE__CT_RESULT;
        msg->mensagem.op_n = last_assigned;
        return 0;
        break;
    }
    case MESSAGE_T__OPCODE__OP_GET:
    { // GET
        pthread_mutex_lock(&arvore);
        key = strdup(msg->mensagem.entry_type->key);
        struct data_t *result = tree_get(tree, key);

        if (result == NULL)
        {
            msg->mensagem.opcode = MESSAGE_T__OPCODE__OP_GET + 1;
            msg->mensagem.c_type = MESSAGE_T__C_TYPE__CT_VALUE;
            msg->mensagem.entry_type->data->datasize = 0;
            msg->mensagem.entry_type->data->data = NULL;
            pthread_mutex_unlock(&arvore);
            return -1;
        }

        msg->mensagem.opcode = MESSAGE_T__OPCODE__OP_GET + 1;
        msg->mensagem.c_type = MESSAGE_T__C_TYPE__CT_VALUE;

        msg->mensagem.entry_type->data->datasize = result->datasize;
        msg->mensagem.entry_type->data->data = result->data;

        free(key);
        pthread_mutex_unlock(&arvore);
        return 0;
        break;
    }
    case MESSAGE_T__OPCODE__OP_PUT:
    { // PUT
        last_assigned++;
        aux = (struct request_t *)malloc(sizeof(struct request_t));
        aux->data = data_create2(msg->mensagem.entry_type->data->datasize, msg->mensagem.entry_type->data->data);
        aux->key = msg->mensagem.entry_type->key;
        aux->op_n = last_assigned;
        aux->op = 1;
        write_list(lista, aux);
        pthread_cond_broadcast(&fila_condition);

        msg->mensagem.opcode = MESSAGE_T__OPCODE__OP_PUT + 1;
        msg->mensagem.c_type = MESSAGE_T__C_TYPE__CT_RESULT;
        msg->mensagem.op_n = last_assigned;
        return 0;
        break;
    }
    case MESSAGE_T__OPCODE__OP_GETKEYS:
    { // GETKEYS
        pthread_mutex_lock(&arvore);
        if (tree_size(tree) <= 0)
        {
            msg->mensagem.opcode = MESSAGE_T__OPCODE__OP_ERROR;
            msg->mensagem.c_type = MESSAGE_T__C_TYPE__CT_NONE;
            return -1;
        }

        char **keys = tree_get_keys(tree);

        msg->mensagem.opcode = MESSAGE_T__OPCODE__OP_GETKEYS + 1;
        msg->mensagem.c_type = MESSAGE_T__C_TYPE__CT_KEYS;

        msg->mensagem.n_keys = tree_size(tree);
        msg->mensagem.keys = keys;
        pthread_mutex_unlock(&arvore);
        return 0;
        break;
    }
    case MESSAGE_T__OPCODE__OP_GETVALUES:
    { // GETVALUES
        pthread_mutex_lock(&arvore);
        void **values = tree_get_values(tree);

        if (values == NULL)
        {
            msg->mensagem.opcode = MESSAGE_T__OPCODE__OP_ERROR;
            msg->mensagem.c_type = MESSAGE_T__C_TYPE__CT_NONE;
            pthread_mutex_unlock(&arvore);
            return -1;
        }
        msg->mensagem.opcode = MESSAGE_T__OPCODE__OP_GETVALUES + 1;
        msg->mensagem.c_type = MESSAGE_T__C_TYPE__CT_VALUES;

        msg->mensagem.n_values = tree_size(tree);
        msg->mensagem.values = (char **)values;
        pthread_mutex_unlock(&arvore);
        return 0;
        break;
    }
    case MESSAGE_T__OPCODE__OP_VERIFY:
    { // VERIFY
        pthread_mutex_lock(&arvore);

        if (verify(msg->mensagem.op_n) == 0) // AINDA NAO ACABOU
        {
            msg->mensagem.opcode = MESSAGE_T__OPCODE__OP_ERROR;
            msg->mensagem.c_type = MESSAGE_T__C_TYPE__CT_NONE;
            pthread_mutex_unlock(&arvore);
            return 0;
        }
        msg->mensagem.opcode = MESSAGE_T__OPCODE__OP_VERIFY + 1; // JA ACABOU
        msg->mensagem.c_type = MESSAGE_T__C_TYPE__CT_RESULT;

        pthread_mutex_unlock(&arvore);
        return 0;
        break;
    }
    default:
    { // 99-ERROR
        return -1;
        break;
    }
    }
    return -1;
}
/* Verifica se a operação identificada por op_n foi executada.
 */
int verify(int op_n)
{
    if (op_n > proc->max_proc)
    {
        return 0;
    }

    for (int i = 0; i < number_threads; i++)
    {
        if (proc->in_progress[i] == op_n)
        {
            return 0;
        }
    }

    return 1;
}

/**********************************************************************************/

void *create_shared_memory(char *name, int size)
{
    int *ptr;
    int ret;
    int fd = shm_open(name, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
    if (fd == -1)
    {
        perror("shm");
        return NULL;
    }
    ret = ftruncate(fd, size);
    if (ret == -1)
    {
        perror("shm");
        return NULL;
    }
    ptr = mmap(0, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (ptr == MAP_FAILED)
    {
        perror("shm-mmap");
        return NULL;
    }
    return ptr;
}

void destroy_shared_memory(char *name, void *ptr, int size)
{
    int ret = munmap(ptr, size);
    if (ret == -1)
        exit(7);

    ret = shm_unlink(name);
    if (ret == -1)
        exit(7);
}

struct circular_list *create_list()
{
    struct circular_list *list = (struct circular_list *)malloc(sizeof(struct circular_list));
    list->buffer = create_shared_memory("buffer", sizeof(list->buffer) * list_size);
    list->ptrs = create_shared_memory("ptrs", sizeof(list->ptrs) * list_size);
    return list;
}

void destroy_list(struct circular_list *list)
{
    destroy_shared_memory("buffer", list->buffer, sizeof(list->buffer) * list_size);
    destroy_shared_memory("ptrs", list->ptrs, sizeof(list->ptrs) * list_size);
    free(list);
}

void write_list(struct circular_list *list, struct request_t *request)
{
    if (((list->ptrs->in + 1) % list_size) == list->ptrs->out)
    {
        return;
    }

    list->buffer[list->ptrs->in] = *request;
    list->ptrs->in = (list->ptrs->in + 1) % list_size;
    return;
}

struct request_t *read_list(struct circular_list *list)
{
    int in = list->ptrs->in;
    int out = list->ptrs->out;
    if (in == out)
    {
        return NULL;
    }

    list->ptrs->out = (out + 1) % list_size;
    return &list->buffer[out];
}

int is_list_empty(struct circular_list *list)
{
    int in = list->ptrs->in;
    int out = list->ptrs->out;
    return in == out;
}