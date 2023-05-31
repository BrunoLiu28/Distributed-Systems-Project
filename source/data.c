/*
    Trabalho realizado pelo grupo 14
    Os elementos que compoem o grupo são:
        Bruno Liu           fc56297
        João Vedor          fc56311
        Rodrigo Cancelinha  fc56371
*/

#include "data.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* Função que cria um novo elemento de dados data_t, reservando a memória
 * necessária para armazenar os dados, especificada pelo parâmetro size 
 */
struct data_t *data_create(int size){
    if(size < 1){
        return NULL;
    }
    struct data_t *newdata = malloc(sizeof (struct data_t));
    if(newdata == NULL){
        free(newdata);
        return NULL;
    }

    newdata->data = malloc(size);
    if(newdata->data == NULL){
        data_destroy(newdata);
        return NULL;
    }
    newdata->datasize = size;
    return newdata;
}

/* Função que cria um novo elemento de dados data_t, inicializando o campo
 * data com o valor passado no parâmetro data, sem necessidade de reservar
 * memória para os dados.
 */
struct data_t *data_create2(int size, void *data){
    if(size < 1){
        return NULL;
    }
    if(data == NULL){
        return NULL;
    }

    struct data_t *aux = malloc(sizeof (struct data_t));
    if(aux == NULL){
        data_destroy(aux);
        return NULL;
    }

    aux->data = data;
    aux->datasize = size;

    return aux;
}

/* Função que elimina um bloco de dados, apontado pelo parâmetro data,
 * libertando toda a memória por ele ocupada.
 */
void data_destroy(struct data_t *data){
    if(data != NULL){
        free(data->data);
        free(data);
    }
    
}

/* Função que duplica uma estrutura data_t, reservando toda a memória
 * necessária para a nova estrutura, inclusivamente dados.
 */ 
struct data_t *data_dup(struct data_t *data){
    if(data == NULL || data->datasize < 1 || data->data == NULL){
        return NULL;
    }

    struct data_t *duplicado = malloc(sizeof (struct data_t));
    if(duplicado == NULL){
        free(duplicado);
        return NULL;
    }
    duplicado->datasize = data->datasize;
    duplicado->data = malloc(data->datasize);
    if(duplicado->data == NULL){
        free(duplicado);
        return NULL;
    }
    memcpy(duplicado->data, data->data, data->datasize);

    return duplicado;
}

/* Função que substitui o conteúdo de um elemento de dados data_t.
*  Deve assegurar que destroi o conteúdo antigo do mesmo.
*/
void data_replace(struct data_t *data, int new_size, void *new_data){
    if(data == NULL || new_size < 1 || new_data == NULL){
        return;
    }
    free(data->data);
    data->datasize = new_size;
    data->data = new_data;
}