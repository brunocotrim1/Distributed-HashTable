/*Realizado por:
-João Ferraz, nº49420.
-Miguel Carvalho, nº54399;
-Bruno Cotrim, nº54406;*/
#include <serialization.h>

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
/* Serializa uma estrutura data num buffer que será alocado
 * dentro da função. Além disso, retorna o tamanho do buffer
 * alocado ou -1 em caso de erro.
 */
int data_to_buffer(struct data_t *data, char **data_buf)
{
    if (data == NULL || data_buf == NULL || *data_buf == NULL)
        return -1;

    int size = sizeof(int) + data->datasize;
    *data_buf = calloc('\0', size + 1);
    memcpy(*data_buf, (const unsigned char *)&(data->datasize), sizeof(int));
    memcpy(*data_buf + sizeof(int), (const unsigned char *)data->data, data->datasize);
    return size;
}

/* De-serializa a mensagem contida em data_buf, com tamanho
 * data_buf_size, colocando-a e retornando-a numa struct
 * data_t, cujo espaco em memoria deve ser reservado.
 * Devolve NULL em caso de erro.
 */
struct data_t *buffer_to_data(char *data_buf, int data_buf_size)
{
    if (data_buf_size <= 0 || data_buf == NULL)
        return NULL;

    int *size = malloc(sizeof(int));

    memcpy(size, (int *)data_buf, sizeof(int));

    char *data = malloc(*size);
    memcpy(data, data_buf + sizeof(int), *size);

    struct data_t *deSerialized = data_create2(*size, data);
    free(size);
    return deSerialized;
}

/* Serializa uma estrutura entry num buffer que sera alocado
 * dentro da função. Além disso, retorna o tamanho deste
 * buffer alocado ou -1 em caso de erro.
 */
int entry_to_buffer(struct entry_t *data, char **entry_buf)
{
    if (data == NULL || entry_buf == NULL || *entry_buf == NULL)
        return -1;

    char *data_buf = "";

    int dataSize = data_to_buffer(data->value, &data_buf);

    int keySize = strlen(data->key) + 1;

    int totalSize = sizeof(int) + keySize + sizeof(int) + data->value->datasize;
    *entry_buf = calloc('\0', totalSize + 1);

    memcpy(*entry_buf, (const unsigned char *)&(keySize), sizeof(int));

    memcpy(*entry_buf + sizeof(int), (const unsigned char *)data->key, keySize);
    memcpy(*entry_buf + keySize + sizeof(int), (const unsigned char *)data_buf, dataSize);

    free(data_buf);
    data_buf = NULL;
    return totalSize;
}

/* De-serializa a mensagem contida em entry_buf, com tamanho
 * entry_buf_size, colocando-a e retornando-a numa struct
 * entry_t, cujo espaco em memoria deve ser reservado.
 * Devolve NULL em caso de erro.
 */
struct entry_t *buffer_to_entry(char *entry_buf, int entry_buf_size)
{
    if (entry_buf_size <= 0 || entry_buf == NULL)
        return NULL;

    struct entry_t *deSerialized = malloc(entry_buf_size);
    int *keySize = malloc(sizeof(int));
    memcpy(keySize, entry_buf, sizeof(int));

    deSerialized->key = malloc(*keySize);

    memcpy(deSerialized->key, entry_buf + sizeof(int), *keySize);

    deSerialized->value = buffer_to_data(entry_buf + *keySize + sizeof(int), entry_buf_size - *keySize - sizeof(int));

    free(keySize);
    keySize = NULL;
    return deSerialized;
}
