/*Realizado por:
-João Ferraz, nº49420.
-Miguel Carvalho, nº54399;
-Bruno Cotrim, nº54406;*/
#include <list.h>
#include "list-private.h"
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
/* Função que cria uma nova lista (estrutura list_t a ser definida pelo
 * grupo no ficheiro list-private.h).
 * Em caso de erro, retorna NULL.
 */
struct list_t *list_create()
{
    struct list_t *ptr = malloc(sizeof(struct list_t));
    if (ptr == NULL)
        return NULL;
    ptr->head = malloc(sizeof(struct node_t));
    ptr->head->data = NULL;
    ptr->head->next = NULL;
    ptr->size = 0;
    return ptr;
}

/* Função que elimina uma lista, libertando *toda* a memoria utilizada
 * pela lista.
 */
void list_destroy(struct list_t *list)
{

    if (list->head == NULL)
    {
        free(list);
        return;
    }
    struct node_t *current = list->head;
    struct node_t *next;
    while (current != NULL)
    {
        next = current->next;
        entry_destroy(current->data);
        free(current);
        current = NULL;
        current = next;
    }
    current = NULL;

    free(list);
    list = NULL;
}

/* Função que adiciona  no final da lista (tail) a entry passada como
* argumento caso não exista na lista uma entry com key igual àquela
* que queremos inserir.
* Caso exista, os dados da entry (value) já existente na lista serão
* substituídos pelos os da nova entry.
* Retorna 0 (OK) ou -1 (erro).
*/

int list_add(struct list_t *list, struct entry_t *entry)
{
    if (list == NULL || entry == NULL)
        return -1;

    if (list->size == 0)
    {
        list->head->data = entry;
        list->size++;
    }
    else
    {
        struct node_t *current = list->head;
        while (current != NULL)
        {

            if (current->data != NULL && entry_compare(current->data, entry) == 0)
            {
                entry_destroy(current->data);
                current->data = NULL;
                current->data = entry;
                break;
            }
            else if (current->next == NULL)
            {
                struct node_t *newNode = malloc(sizeof(struct node_t));
                current->next = newNode;
                newNode->next = NULL;
                newNode->data = entry;
                list->size++;
                break;
            }
            current = current->next;
        }
    }
    return 0;
}

/* Função que elimina da lista a entry com a chave key.
 * Retorna 0 (OK) ou -1 (erro).
 */
int list_remove(struct list_t *list, char *key)
{
    struct entry_t *existingData = list_get(list, key);
    if (existingData == NULL)
    {
        entry_destroy(existingData);
        return -1;
    }

    struct node_t *current = list->head, *previous;
    if (current != NULL && strcmp(key, current->data->key) == 0)
    {
        if (current != NULL && current->next != NULL)
            list->head = current->next;
        else
            list->head = calloc(1, sizeof(struct node_t));
        entry_destroy(current->data);
        free(current);
        list->size--;
        return 0;
    }
    while (current != NULL && strcmp(key, current->data->key) != 0)
    {

        previous = current;
        current = current->next;
    }

    if (current == NULL)
        return -1;
    entry_destroy(current->data);
    free(current);
    list->size--;

    return 0;
}

/* Função que obtém da lista a entry com a chave key.
 * Retorna a referência da entry na lista ou NULL em caso de erro.
 * Obs: as funções list_remove e list_destroy vão libertar a memória
 * ocupada pela entry ou lista, significando que é retornado NULL
 * quando é pretendido o acesso a uma entry inexistente.
*/
struct entry_t *list_get(struct list_t *list, char *key)
{

    if (list == NULL || key == NULL || list->size == 0)
        return NULL;
    struct node_t *current = list->head;

    while (current != NULL)
    {
        if (current->data != NULL && current->data->key != NULL && strcmp(current->data->key, key) == 0)
        {
            return current->data == NULL ? NULL : current->data;
        }
        current = current->next;
    }

    return NULL;
}

/* Função que retorna o tamanho (número de elementos (entries)) da lista,
 * ou -1 (erro).
 */
int list_size(struct list_t *list)
{
    return list == NULL ? -1 : list->size;
}

/* Função que devolve um array de char* com a cópia de todas as keys da 
 * tabela, colocando o último elemento do array com o valor NULL e
 * reservando toda a memória necessária.
 */
char **list_get_keys(struct list_t *list)
{
    if (list == NULL || list->size == 0 || list->head == NULL)
        return NULL;
    char **keys = malloc((sizeof(char *) * (list->size + 1)));
    struct node_t *current = list->head;
    int i = 0;
    while (current != NULL)
    {
        keys[i] = malloc(strlen(current->data->key) + 1);
        strcpy(keys[i], current->data->key);
        current = current->next;
        i++;
    }
    keys[i] = NULL;
    return keys;
}

/* Função que liberta a memória ocupada pelo array das keys da tabela,
 * obtido pela função list_get_keys.
 */
void list_free_keys(char **keys)
{
    char *iterator = keys[0];
    int i = 0;
    while (iterator != NULL)
    {
        free(iterator);
        i++;
        iterator = keys[i];
    }
    free(iterator);
    free(keys);
}

/* Função que imprime o conteúdo da lista para o terminal.
 */
void list_print(struct list_t *list)
{
    if (list->size == 0)
        return;
    struct node_t *current = list->head;

    printf("[");
    while (current != NULL)
    {
        printf("(%s,%s)", current->data->key, (char *)current->data->value->data);

        current = current->next;
    }
    printf("]\n");
}
