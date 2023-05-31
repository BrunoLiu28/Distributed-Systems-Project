/*
    Trabalho realizado pelo grupo 14
    Os elementos que compoem o grupo são:
        Bruno Liu           fc56297
        João Vedor          fc56311
        Rodrigo Cancelinha  fc56371
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "entry.h"
#include "data.h"
#include "client_stub.h"
#include "client_stub-private.h"
#include <zookeeper/zookeeper.h>

typedef struct String_vector zoo_string;
struct rtree_t *head;
struct rtree_t *tail;
static zhandle_t *zh;
static int is_connected;
static char *root_path = "/chain";
static char *watcher_ctx = "ZooKeeper Data Watcher";
char *client_ip;

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
      char *highest;
      char *lowest;
      for (int i = 0; i < children_list->count; i++)
      {
        if (i == 0)
        {
          highest = children_list->data[i];
          lowest = children_list->data[i];
        }
        else
        {
          if (strcmp(highest, children_list->data[i]) < 0)
          {
            highest = children_list->data[i];
          }
          else if (strcmp(lowest, children_list->data[i]) > 0)
          {
            lowest = children_list->data[i];
          }
        }
      }

      char path[120] = "";
      strcat(path, root_path);
      strcat(path, "/");
      strcat(path, highest);
      int highest_length = 128;
      char highest_buffer[highest_length];
      zoo_get(zh, path, 0, highest_buffer, &highest_length, NULL);

      char highest_aux[128];
      strcpy(highest_aux, client_ip);
      strcat(highest_aux, ":");
      strcat(highest_aux, highest_buffer);
      tail = rtree_connect(highest_aux);

      //--------------------------------------
      char path2[120] = "";
      strcat(path2, root_path);
      strcat(path2, "/");
      strcat(path2, lowest);
      char highest_buffer2[highest_length];
      zoo_get(zh, path2, 0, highest_buffer2, &highest_length, NULL);

      char lowest_aux[128];
      strcpy(lowest_aux, client_ip);
      strcat(lowest_aux, ":");
      strcat(lowest_aux, highest_buffer2);
      head = rtree_connect(lowest_aux);
    }
  }
  free(children_list);
}

int main(int argc, char *argv[])
{
  if(argc != 2){
        printf("Syntax do argumentos esta mal, deve ser ./tree-client <<IP>:<porta ZooKeeper>>\n");
        return -1;
    }

  printf("Bem Vindo\n");
  zoo_string *children_list = NULL;

  zh = zookeeper_init(argv[1], connection_watcher, 2000, 0, 0, 0);

  char *client_ip_aux = (char *)malloc(sizeof(argv[1]));
  strcpy(client_ip_aux, argv[1]);
  client_ip = strtok(client_ip_aux, ":");

  children_list = (zoo_string *)malloc(sizeof(zoo_string));
  if (ZOK != zoo_wget_children(zh, root_path, &child_watcher, watcher_ctx, children_list))
  {
    fprintf(stderr, "Error setting watch at %s!\n", root_path);
  }

  char *highest;
  char *lowest;
  for (int i = 0; i < children_list->count; i++)
  {
    if (i == 0)
    {
      highest = children_list->data[i];
      lowest = children_list->data[i];
    }
    else
    {

      if (strcmp(highest, children_list->data[i]) < 0)
      {
        highest = children_list->data[i];
      }
      else if (strcmp(lowest, children_list->data[i]) > 0)
      {
        lowest = children_list->data[i];
      }
    }
  }

  //TAIL
  char path[120] = "";
  strcat(path, root_path);
  strcat(path, "/");
  strcat(path, highest); 
  int highest_length = 128;
  char highest_buffer[highest_length];
  zoo_get(zh, path, 0, highest_buffer, &highest_length, NULL);
  
  char highest_aux[128];
  strcpy(highest_aux, client_ip);
  strcat(highest_aux, ":");
  strcat(highest_aux, highest_buffer);
  tail = rtree_connect(highest_aux);

  //HEAD
  char path2[120] = "";
  strcat(path2, root_path);
  strcat(path2, "/");
  strcat(path2, lowest); 
  char highest_buffer2[highest_length];
  zoo_get(zh, path2, 0, highest_buffer2, &highest_length, NULL);

  char lowest_aux[128];
  strcpy(lowest_aux, client_ip);
  strcat(lowest_aux, ":");
  strcat(lowest_aux, highest_buffer2);
  head = rtree_connect(lowest_aux);

  if (head != NULL && tail != NULL)
  {
    int out = 0;
    do
    {
      char *comando;
      printf("______________________________________________________________________\n");
      printf("Escreva o comando que deseja realizar, com a syntax dos exemplos abaixo:\n");
      printf("\t- put <key> <data>\n");
      printf("\t- get <key>\n");
      printf("\t- del <key>\n");
      printf("\t- size\n");
      printf("\t- height\n");
      printf("\t- getkeys\n");
      printf("\t- getvalues\n");
      printf("\t- verify <op_n>\n");
      printf("\t- quit\n");
      char pedido[200];
      fgets(pedido, 200, stdin);
      if ( strcmp(pedido, "\n") != 0)
      {
        comando = strtok(pedido, " \n");

        if (strcmp(comando, "put") == 0)
        {
          char *key = strtok(NULL, " ");
          void *data_temp = strtok(NULL, " ");
          void *data_input = strtok(data_temp, "\n");
          char *finalData = malloc(strlen(data_input) + 1);
          strcpy(finalData, data_temp);
          struct data_t *data = data_create2(strlen(finalData) + 1, finalData);
          struct entry_t *entry = entry_create(key, data);
          rtree_put(head, entry);
        }
        else if (strcmp(comando, "get") == 0)
        {

          char *key_temp = strtok(NULL, " ");
          char *key_input = strtok(key_temp, "\n");

          char *finalKey = malloc(strlen(key_input) + 1);
          strcpy(finalKey, key_input);

          struct data_t *result = rtree_get(tail, finalKey);
          if (result == NULL)
          {
            printf("A chave que inseriu nao está presente da arvore, tente novamente\n");
          }
          else
          {
            printf("O conteudo da chave é: %s \n", (char *)result->data);
          }

        }
        else if (strcmp(comando, "del") == 0)
        {

          char *key_temp = strtok(NULL, " ");
          char *key_input = strtok(key_temp, "\n");

          char *key = malloc(strlen(key_input) + 1);

          strcpy(key, key_input);
          rtree_del(head, key);
          free(key);
        }
        else if (strcmp(comando, "size") == 0)
        {
          int tamanho = 0;
          if ((tamanho = rtree_size(tail)) == -1)
          {
            printf("Ocorreu um erro a realizar a operacao size\n");
          }
          else
          {
            printf("O tamanho da arvore é: %d\n", tamanho);
          }
        }
        else if (strcmp(comando, "height") == 0)
        {

          int altura = 0;
          if ((altura = rtree_height(tail)) == -1)
          {
            printf("Ocorreu um erro a realizar a operacao Height\n");
          }
          else
          {
            printf("A altura da arvore é: %d\n", altura);
          }
        }
        else if (strcmp(comando, "getkeys") == 0)
        {

          char **keys = (char **)rtree_get_keys(tail);
          if (keys != NULL)
          {
            printf("As chaves que estao na arvore sao:\n");
            for (int i = 0; keys[i] != NULL; i++)
            {
              if (keys[i] != NULL)
                printf("%s\n", keys[i]);
            }
          }
        }
        else if (strcmp(comando, "getvalues") == 0)
        {
          struct data_t *values_aux;
          char **keys = (char **)rtree_get_keys(tail);
          if (keys != NULL)
          {
            printf("Os valores que estao na arvore sao:\n");
            for (int i = 0; keys[i] != NULL; i++)
            {
              if (keys[i] != NULL)
              {
                values_aux = rtree_get(tail, keys[i]);
                printf("%s\n", (char *)values_aux->data);
              }
            }
          }
        }
        else if (strcmp(comando, "verify") == 0)
        {
          char *key_temp = strtok(NULL, " ");
          char *key_input = strtok(key_temp, "\n");

          char *finalKey = malloc(strlen(key_input) + 1);
          strcpy(finalKey, key_input);
          if (atoi(finalKey) < 1)
          {
            printf("Os pedidos começam no numero 1, tente novamente\n");
          }
          else
          {
            rtree_verify(tail, atoi(finalKey));
            if (rtree_verify(tail, atoi(finalKey)) == -1)
            {
              printf("A operacao ainda nao foi finalizada\n");
            }
            else
            {
              printf("A operacao ja foi finalizada\n");
            }
          }
        }
        else if (strcmp(comando, "quit") == 0)
        {
          printf("A fechar ligacao com o servidor\n");
          rtree_disconnect(head);
          rtree_disconnect(tail);
          out = 1;
        }
        else
        {
          printf("Syntax errada ou comando inexistente, tente novamente\n");
        }
      } else {
        printf("Syntax errada ou comando inexistente, tente novamente\n");
      }
    } while (out != 1);
    return 0;
  }

  return 0;
}