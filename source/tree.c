/*
    Trabalho realizado pelo grupo 14
    Os elementos que compoem o grupo são:
        Bruno Liu           fc56297
        João Vedor          fc56311
        Rodrigo Cancelinha  fc56371
*/

#include "data.h"
#include "tree.h"
#include "tree-private.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* Função para criar uma nova árvore tree vazia.
 * Em caso de erro retorna NULL.
 */
struct tree_t *tree_create(){
    struct tree_t *arvore = malloc(sizeof(struct tree_t));
    if(arvore == NULL){
        tree_destroy(arvore);
        return NULL;
    }
    arvore->root = malloc(sizeof(struct node));
    if(arvore->root == NULL){
        tree_destroy(arvore);
        return NULL;
    }

    arvore->root->entry = NULL;
    arvore->root->left = NULL;
    arvore->root->right = NULL;

    arvore->size = 0;

    return arvore;
}

/* Função para libertar toda a memória ocupada por uma árvore.
 */
void tree_destroy(struct tree_t *tree){
    if (tree != NULL) {
        tree_destroy_aux((tree->root));
        free(tree);
    }
}

/* Função para adicionar um par chave-valor à árvore.
 * Os dados de entrada desta função deverão ser copiados, ou seja, a
 * função vai *COPIAR* a key (string) e os dados para um novo espaço de
 * memória que tem de ser reservado. Se a key já existir na árvore,
 * a função tem de substituir a entrada existente pela nova, fazendo
 * a necessária gestão da memória para armazenar os novos dados.
 * Retorna 0 (ok) ou -1 em caso de erro.
 */
int tree_put(struct tree_t *tree, char *key, struct data_t *value){

   if(tree == NULL || key == NULL || value == NULL){
        return -1;
    }
    struct entry_t *auxiliar = entry_create(strdup(key), data_dup(value));

    int result = tree_put_aux(&(tree->root), auxiliar);

    if(result == 0 || result == 1){
        tree->size += result;
    } else {
        return -1;
    }
    
    return 0;
}

/* Função para obter da árvore o valor associado à chave key.
 * A função deve devolver uma cópia dos dados que terão de ser
 * libertados no contexto da função que chamou tree_get, ou seja, a
 * função aloca memória para armazenar uma *CÓPIA* dos dados da árvore,
 * retorna o endereço desta memória com a cópia dos dados, assumindo-se
 * que esta memória será depois libertada pelo programa que chamou
 * a função. Devolve NULL em caso de erro.
 */
struct data_t *tree_get(struct tree_t *tree, char *key){
      if(tree == NULL || key == NULL ){
        return NULL;
    }  

    return tree_get_aux (&(tree->root), key);
}


/* Função para remover um elemento da árvore, indicado pela chave key,
 * libertando toda a memória alocada na respetiva operação tree_put.
 * Retorna 0 (ok) ou -1 (key not found).
 */
int tree_del(struct tree_t *tree, char *key){

    int result = tree_del_aux(&tree->root, key);
    if(result == 0){
        tree->size -= 1;
        return 0;
    }
    return -1;
}


/* Função que devolve o número de elementos contidos na árvore.
 */
int tree_size(struct tree_t *tree){
    return tree->size;
}

/* Função que devolve a altura da árvore.
 */
int tree_height(struct tree_t *tree){  
    return tree_height_aux(tree->root);
}

/* Função que devolve um array de char* com a cópia de todas as keys da
 * árvore, colocando o último elemento do array com o valor NULL e
 * reservando toda a memória necessária. As keys devem vir ordenadas segundo a ordenação lexicográfica das mesmas.
 */
char **tree_get_keys(struct tree_t *tree){

    int counter = 0;
    char **keys = malloc((tree->size+1)*sizeof(char*));
    if(keys == NULL){
        free(keys);
        return NULL;
    }

    if(tree->size > 0){
        tree_get_keys_aux(keys, tree->root, counter);
    }

    keys[tree->size] = NULL; 

    return keys;
}

/* Função que devolve um array de void* com a cópia de todas os values da
 * árvore, colocando o último elemento do array com o valor NULL e
 * reservando toda a memória necessária.
 */
void **tree_get_values(struct tree_t *tree){

    int counter = 0; 
    void **valores = malloc((tree->size+1)*sizeof(struct data_t*));

    if(valores == NULL){
        free(valores);
        return NULL;
    }
    
    if(tree->size > 0){
        tree_get_values_aux(valores, tree->root, counter); 
    }
    
    valores[tree->size] = NULL;  

    return valores;
}

/* Função que liberta toda a memória alocada por tree_get_keys().
 */
void tree_free_keys(char **keys){
    for (int i = 0; keys[i] != NULL; i++)
    {
        free(keys[i]);
    }
    free(keys);  
}

/* Função que liberta toda a memória alocada por tree_get_values().
 */
void tree_free_values(void **values){
    for (int i = 0; values[i] != NULL; i++)
    {
        free(values[i]);
    }
    free(values);
}


/*Funcoes auxiliares*/
void tree_destroy_aux(struct node *node){

    if(!(node->entry)){
        free(node);
        return;
    }


    tree_destroy_aux((node->left));
    tree_destroy_aux((node->right));

    entry_destroy(node->entry);
    free(node);
    return;
}

int tree_put_aux(struct node **node, struct entry_t *new_entry){

    struct node *nodeaux = *node;

    if((nodeaux->entry == NULL)){
        (nodeaux->entry) = new_entry;
        
        nodeaux->left = malloc(sizeof(struct node));
        if(nodeaux->left == NULL){
            return -1;
        }
        nodeaux->left->entry= NULL;

        nodeaux->right = malloc(sizeof(struct node));
        if(nodeaux->right == NULL){
            return -1;
        }
        nodeaux->right->entry= NULL;
        return 1;
    } 
    int comparation = entry_compare(new_entry,nodeaux->entry);

    if(comparation == -1){
        return tree_put_aux(&(nodeaux->left), new_entry);
    } else if(comparation == 1){
        return tree_put_aux(&(nodeaux->right), new_entry);
    } else {
        entry_replace(nodeaux->entry, new_entry->key, new_entry->value);
        return 0;
    }
    return 0;
}

struct data_t *tree_get_aux(struct node **node, char *key){
    struct node *nodeaux = *node;
    if(nodeaux->entry == NULL){
        return NULL;
    } 
    if(nodeaux->entry->key == NULL){
        return NULL;
    }
    int comparation = key_compare(key,nodeaux->entry);
    if(comparation == -1){
        return tree_get_aux(&(nodeaux->left), key);
    } else if(comparation == 1){
        return tree_get_aux(&(nodeaux->right), key);
    } else {
       return data_dup(nodeaux->entry->value);
    }
    return NULL;
}

int key_compare(char *key, struct entry_t *entry2){
    int result = strcmp(key, entry2->key);

    if(result > 0){
        return 1;
    } else if(result < 0){
        return -1;
    }
    return 0;
}

int tree_del_aux(struct node **node, char *key){
    struct node *node2 = *node;
    if (node2->entry == NULL){
       return -1;
    }
    int compare = key_compare(key, node2->entry);
    if(compare < 0){
        if(node2->left->entry == NULL){
            return -1;
        }
        return tree_del_aux(&node2->left, key);
        
    } else if (compare > 0){
        if(node2->right->entry == NULL){
            return -1;
        }
        return tree_del_aux(&node2->right, key);
    } else {
        if(node2->left->entry == NULL){
            free(node2->left);
            entry_destroy(node2->entry);
            *node = node2->right;
            return 0;
        } else if(node2->right->entry == NULL){
            free(node2->right);
            entry_destroy(node2->entry);
            *node = node2->left;
            return 0;
        } else {
            if(node2->left->right->entry == NULL){
                struct node *aux = node2->left;
                aux->right = node2->right;
                entry_destroy(node2->entry);
                node2 = aux;
                return 0;
            } else {
                struct node *aux = remove_largest_child(&(node2->left));

                entry_destroy(node2->entry);
                node2->entry = entry_dup(aux->entry);

                return 0;
            }
        }
    }

}

struct node *remove_largest_child(struct node **node){
    struct node *node2 = *node;
    struct node *aux2;
    if(node2->right->right->entry == NULL){
        aux2 = node2->right;
        if(node2->right == NULL){
            (*node)->right = NULL;
        } else {
            node2->right = node2->right->left;
        }

        return aux2;
    } else {
        return remove_largest_child(&(node2->right));
    }
}

int tree_height_aux(struct node *node){
    if (node->entry == NULL){
        return 0;
    }
    
    return 1 + max(tree_height_aux(node->left), tree_height_aux(node->right));
}

int tree_get_keys_aux(char **chaves, struct node *root, int counter){ 
    if(root->entry == NULL){
        return counter;
    }
    
    if(root->left != NULL){
        counter = tree_get_keys_aux(chaves,root->left, counter);
    }
    
    chaves[counter] = strdup(root->entry->key);
    counter ++;

    if(root->right != NULL){
        counter = tree_get_keys_aux(chaves,root->right, counter);
    }

    return counter;
}

int tree_get_values_aux(void **valores, struct node *root, int counter){
    if(root->entry == NULL){
        return counter;
    }

    if(root->left != NULL){
        counter = tree_get_values_aux(valores,root->left, counter);
    }
    
    valores[counter] = data_dup(root->entry->value);
    counter ++;

    if(root->right != NULL){
        counter = tree_get_values_aux(valores,root->right, counter);
    }
    return 0;
}

int max(int num1, int num2){
    return (num1 > num2 ) ? num1 : num2;
}