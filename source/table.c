/*Realizado por:
-João Ferraz, nº49420.
-Miguel Carvalho, nº54399;
-Bruno Cotrim, nº54406;*/
#include <table.h>
#include "table-private.h"
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
/* Função para criar/inicializar uma nova tabela hash, com n
 * linhas (n = módulo da função hash)
 * Em caso de erro retorna NULL.
 */
struct table_t *table_create(int n)
{
    if (n <= 0)
        return NULL;
    struct table_t *table = malloc(sizeof(struct table_t));
    table->buckets = malloc(sizeof(struct list_t *) * n);

    for (int i = 0; i < n; i++)
    {
        table->buckets[i] = list_create();
    }
    //Perguntar se e necessario alocar logo tudo
    table->size = n;
    table->n_elements = 0;
    return table;
}

void table_destroy(struct table_t *table)
{

    for (int i = 0; i < table->size; i++)
    {

        list_destroy(table->buckets[i]);
    }
    free(table->buckets);
    table->buckets = NULL;
    free(table);
    table = NULL;
}

int hashTabela(char *key, int m)
{

    long hashCode = 0;
    unsigned char *keyV = key;

    while (*keyV != '\0')
    {
        hashCode = (hashCode * 31 + *keyV) % m;
        keyV++;
    }
    return (int)hashCode;
}

/* Função para adicionar um par chave-valor à tabela.
 * Os dados de entrada desta função deverão ser copiados, ou seja, a
 * função vai *COPIAR* a key (string) e os dados para um novo espaço de
 * memória que tem de ser reservado. Se a key já existir na tabela,
 * a função tem de substituir a entrada existente pela nova, fazendo
 * a necessária gestão da memória para armazenar os novos dados.
 * Retorna 0 (ok) ou -1 em caso de erro.
 */
int table_put(struct table_t *table, char *key, struct data_t *value)
{
    if (table == NULL || key == NULL || value == NULL)
        return -1;
    struct data_t *existingData = table_get(table, key);
    if (existingData != NULL)
    {
        table->n_elements--;
        data_destroy(existingData);
    }

    struct data_t *dataCopy = data_dup(value);

    char *keyCopy = malloc(strlen(key) + 1);

    strcpy(keyCopy, key);

    struct entry_t *entry = entry_create(keyCopy, dataCopy);

    int hashCode = hashTabela(key, table->size);
    list_add(table->buckets[hashCode], entry);
    table->n_elements++;
    return 0;
}

/* Função para obter da tabela o valor associado à chave key.
 * A função deve devolver uma cópia dos dados que terão de ser
 * libertados no contexto da função que chamou table_get, ou seja, a
 * função aloca memória para armazenar uma *CÓPIA* dos dados da tabela,
 * retorna o endereço desta memória com a cópia dos dados, assumindo-se
 * que esta memória será depois libertada pelo programa que chamou
 * a função.
 * Devolve NULL em caso de erro.
 */
struct data_t *table_get(struct table_t *table, char *key)
{

    if (key == NULL || table == NULL)
        return NULL;
    struct entry_t *entry = list_get(table->buckets[hashTabela(key, table->size)], key);

    if (entry == NULL)
        return NULL;

    struct data_t *data = data_dup(entry->value);
    return data;
}

/* Função para remover um elemento da tabela, indicado pela chave key, 
 * libertando toda a memória alocada na respetiva operação table_put.
 * Retorna 0 (ok) ou -1 (key not found).
 */
int table_del(struct table_t *table, char *key)
{
    if (table == NULL || key == NULL)
        return -1;

    int r = list_remove(table->buckets[hashTabela(key, table->size)], key);
    if (r == 0)
    {
        table->n_elements--;
        return r;
    }

    return -1;
}

/* Função que devolve o número de elementos contidos na tabela.
 */
int table_size(struct table_t *table)
{
    return table->n_elements;
}

/* Função que devolve um array de char* com a cópia de todas as keys da
 * tabela, colocando o último elemento do array com o valor NULL e
 * reservando toda a memória necessária.
 */
char **table_get_keys(struct table_t *table)
{

    if (table == NULL)
        return NULL;
    char **keys = malloc(sizeof(char *) * (table->n_elements + 1));
    int i = 0;
    int offset = 0;
    while (i < table->size)
    {
        if (table->buckets[i] != NULL)
        {
            char **listKeys = list_get_keys(table->buckets[i]);
            if (listKeys != NULL)
            {

                memcpy(keys + offset, listKeys, table->buckets[i]->size * sizeof(char *));
                offset += table->buckets[i]->size;
                free(listKeys[table->buckets[i]->size]);
                free(listKeys);
            }
        }
        i++;
    }
    keys[table->n_elements] = NULL;
    return keys;
}

/* Função que liberta toda a memória alocada por table_get_keys().
 */
void table_free_keys(char **keys)
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

/* Função que imprime o conteúdo da tabela.
 */
char *table_print(struct table_t *table)
{
    if (table->n_elements == 0)
        return NULL;
    int len = 2;
    char *keysToStr = (char *)calloc('\0', len);
    keysToStr[0] = '{';
    for (int i = 0; i < table->size; i++)
    {
        if (table->buckets[i] != NULL)
        {
            struct node_t *current = table->buckets[i]->head;
            while (current != NULL)
            {

                int size = snprintf(NULL, 0, "%d \n", i);
                char *toConcatenate = malloc(size + 1);

                if (current != table->buckets[i]->head)
                    sprintf(toConcatenate, "%s", "");
                if (current->data != NULL && current->data->key != NULL && current->data->value != NULL)
                {
                    sprintf(toConcatenate, "%s=%s,", current->data->key, (char *)current->data->value->data);
                }

                len += 1 + strlen(toConcatenate);
                keysToStr = (char *)realloc(keysToStr, len);
                strncat(keysToStr, toConcatenate, len);
                free(toConcatenate);
                current = current->next;
            }
        }
    }

    keysToStr[strlen(keysToStr) - 1] = '}';
    return keysToStr;
}
