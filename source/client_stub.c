/*Realizado por:
-João Ferraz, nº49420.
-Miguel Carvalho, nº54399;
-Bruno Cotrim, nº54406;
*/
#include <fcntl.h>  // for open
#include <unistd.h> // for close
#include "client_stub.h"
#include "client_stub-private.h"
#include "stats.h"
#include "network_client.h"
#include "serialization.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "network_server.h"
static int is_connected;
static zhandle_t *zh;
typedef struct String_vector zoo_string;
int ZDATALEN = 1024 * 1024;
struct rtable_t *rTable;
void client_connection_watcher(zhandle_t *zzh, int type, int state, const char *path, void *context)
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

int ClientConnectToZookeeper(const char *address_port)
{
    zh = zookeeper_init(address_port, client_connection_watcher, 2000, 0, NULL, 0);
    if (zh == NULL)
    {
        fprintf(stderr, "Error connecting to ZooKeeper server!\n");
        return -1;
    }
    return 0;
}
static void client_watcher_callback(zhandle_t *wzh, int type, int state, const char *zpath, void *watcher_ctx)
{
    if (state == ZOO_CONNECTED_STATE)
    {
        if (type == ZOO_CHILD_EVENT)
        {
            int existsKvPrimary = zoo_exists(zh, "/kvstore/primary", 0, NULL);
            int existsKvBackup = zoo_exists(zh, "/kvstore/backup", 0, NULL);
            if (existsKvBackup != ZOK)
            {
                printf("BACKUP SERVER DOES NOT EXIST\n");
            }
            else
            {
                printf("BACKUP SERVER EXISTS YOU ARE ALLOWED TO WRITE INTO THE TABLE!\n");
            }
            if (existsKvPrimary != ZOK)
            {
                printf("Closing Connection to dead primary!\n");
                close(rTable->socketfd);
                free(rTable->adress);
                rTable->adress = NULL;
                rTable->port = -1;
                rTable->socketfd = -1;
            }
            else if (existsKvPrimary == ZOK && existsKvBackup != ZOK && rTable->socketfd == -1)
            {
                char *zdata_buf = (char *)calloc(1, ZDATALEN * sizeof(char));
                if (ZOK != zoo_get(zh, "/kvstore/primary", 0, zdata_buf, &ZDATALEN, NULL))
                {
                    printf("ERROR GETTING PRIMARY INFO\n");
                    free(zdata_buf);
                }
                else
                {
                    printf("New Primary Server Address: %s\n", zdata_buf);
                    free(rTable->adress);
                    char *token = strtok(zdata_buf, ":");

                    if (token == NULL)
                        return;

                    rTable->adress = token;

                    token = strtok(NULL, ":");

                    short port = atoi(token);

                    if (token == NULL || port == 0)
                        return;

                    rTable->port = port;

                    if (network_connect(rTable) == -1)
                    {
                        rtable_disconnect(rTable);
                        return;
                    }
                }
            }
            if (ZOK != zoo_wget_children(zh, "/kvstore", client_watcher_callback, "ZooKeeper Data Watcher", NULL))
            {
                fprintf(stderr, "Error setting watch at /kvstore!\n");
            }
        }
    }
}
char *connectToPrimaryServer()
{
    int existsKvPrimary = zoo_exists(zh, "/kvstore/primary", 0, NULL);
    int existsKvBackUp = zoo_exists(zh, "/kvstore/backup", 0, NULL);
    char *zdata_buf = (char *)calloc(1, ZDATALEN * sizeof(char));
    if (existsKvBackUp != ZOK)
    {
        printf("BACKUP SERVER DOES NOT EXIST\n");
    }
    if (existsKvPrimary == ZOK)
    {
        if (ZOK != zoo_get(zh, "/kvstore/primary", 0, zdata_buf, &ZDATALEN, NULL))
        {
            printf("ERROR GETTING PRIMARY INFO\n");
            free(zdata_buf);
            return NULL;
        }
        else
        {
            if (ZOK != zoo_wget_children(zh, "/kvstore", &client_watcher_callback, "ZooKeeper Data Watcher", NULL))
            {
                fprintf(stderr, "Error setting watch at /kvstore!\n");
            }
            return zdata_buf;
        }
    }
    else
    {
        free(zdata_buf);
        return NULL;
    }
}

/* Função para estabelecer uma associação entre o cliente e o servidor, 
 * em que address_port é uma string no formato <hostname>:<port>.
 * Retorna NULL em caso de erro.
 */
struct rtable_t *rtable_connect(const char *address_port)
{
    if (ClientConnectToZookeeper(address_port) == -1)
        return NULL;
    if (address_port == NULL)
        return NULL;

    char *cpy = connectToPrimaryServer();
    if (cpy == NULL)
        return NULL;
    printf("Primary Server Address: %s\n", cpy);
    rTable = malloc(sizeof(struct rtable_t));

    char *token = strtok(cpy, ":");

    if (token == NULL)
        return NULL;

    rTable->adress = token;

    token = strtok(NULL, ":");

    short port = atoi(token);

    if (token == NULL || port == 0)
        return NULL;

    rTable->port = port;

    if (network_connect(rTable) == -1)
    {
        rtable_disconnect(rTable);
        return NULL;
    }

    //free(cpy);
    return rTable;
}

/* Termina a associação entre o cliente e o servidor, fechando a 
 * ligação com o servidor e libertando toda a memória local.
 * Retorna 0 se tudo correr bem e -1 em caso de erro.
 */
int rtable_disconnect(struct rtable_t *rtable)
{
    if (rtable == NULL)
        return -1;
    int status = network_close(rtable);
    free(rtable->adress);
    free(rtable);
    return status;
}

/* Função para adicionar um elemento na tabela.
 * Se a key já existe, vai substituir essa entrada pelos novos dados.
 * Devolve 0 (ok, em adição/substituição) ou -1 (problemas).
 */
int rtable_put(struct rtable_t *rtable, struct entry_t *entry)
{

    if (rtable == NULL || entry == NULL)
        return -1;

    MessageT msg;
    message_t__init(&msg);

    msg.opcode = MESSAGE_T__OPCODE__OP_PUT;
    msg.c_type = MESSAGE_T__C_TYPE__CT_ENTRY;

    msg.n_entries = 1;
    msg.entries = malloc(sizeof(MessageT__Entry *));
    msg.entries[0] = malloc(sizeof(MessageT__Entry));
    message_t__entry__init(msg.entries[0]);
    msg.entries[0]->key = entry->key;
    msg.entries[0]->data.data = (uint8_t *)entry->value->data;
    msg.entries[0]->data.len = entry->value->datasize;
    MessageT *response;
    if ((response = network_send_receive(rtable, &msg)) == NULL)
    {
        free(msg.keys);
        free(msg.entries[0]);
        free(msg.entries);
        message_t__free_unpacked(response, NULL);
        return -1;
    }
    printf("Opcode:%d - c_type%d\n", response->opcode, response->c_type);
    if (response->opcode != MESSAGE_T__OPCODE__OP_PUT + 1 || response->c_type != MESSAGE_T__C_TYPE__CT_NONE || response->opcode == MESSAGE_T__OPCODE__OP_ERROR)
    { //&& response->data == NULL
        message_t__free_unpacked(response, NULL);
        return -1;
    }
    message_t__free_unpacked(response, NULL);

    free(msg.entries[0]);
    free(msg.entries);
    return 0;
}

/* Função para obter um elemento da tabela.
 * Em caso de erro, devolve NULL.
 */
struct data_t *rtable_get(struct rtable_t *rtable, char *key)
{

    if (rtable == NULL || key == NULL)
        return NULL;

    MessageT msg;
    message_t__init(&msg);

    msg.opcode = MESSAGE_T__OPCODE__OP_GET;
    msg.c_type = MESSAGE_T__C_TYPE__CT_KEY;
    msg.data.data = key;
    msg.data.len = strlen(key) + 1;
    msg.data_size = strlen(key) + 1;

    MessageT *response;
    if ((response = network_send_receive(rtable, &msg)) == NULL)
    {
        message_t__free_unpacked(response, NULL);
        return NULL;
    }
    printf("Opcode:%d - c_type%d\n", response->opcode, response->c_type);
    if (response->opcode != MESSAGE_T__OPCODE__OP_GET + 1 || response->c_type != MESSAGE_T__C_TYPE__CT_VALUE || response->opcode == MESSAGE_T__OPCODE__OP_ERROR || response->c_type == MESSAGE_T__C_TYPE__CT_NONE)
    { //&& response->data == NULL
        message_t__free_unpacked(response, NULL);
        return NULL;
    }
    if (response->data.len == 0)
    {
        char *notFoundValue = malloc(1);
        message_t__free_unpacked(response, NULL);
        return data_create2(0, notFoundValue);
    }
    //CRIAR UM DATA_T de acordo com os dados RECEBIDOS
    char *dataValue = malloc(response->data.len + 1);
    memcpy(dataValue, response->data.data, response->data.len);
    struct data_t *data = data_create2(response->data.len, dataValue);

    message_t__free_unpacked(response, NULL);
    return data;
}

/* Função para remover um elemento da tabela. Vai libertar 
 * toda a memoria alocada na respetiva operação rtable_put().
 * Devolve: 0 (ok), -1 (key not found ou problemas).
 */
int rtable_del(struct rtable_t *rtable, char *key)
{
    if (rtable == NULL || key == NULL)
        return -1;

    MessageT msg;
    message_t__init(&msg);
    msg.opcode = MESSAGE_T__OPCODE__OP_DEL;
    msg.c_type = MESSAGE_T__C_TYPE__CT_KEY;
    msg.data.data = key;
    msg.data.len = strlen(key) + 1;

    MessageT *response;
    if ((response = network_send_receive(rtable, &msg)) == NULL)
    {
        message_t__free_unpacked(response, NULL);
        return -1;
    }

    printf("Opcode:%d - c_type%d\n", response->opcode, response->c_type);
    if (response->opcode != MESSAGE_T__OPCODE__OP_DEL + 1 || response->c_type != MESSAGE_T__C_TYPE__CT_NONE || response->opcode == MESSAGE_T__OPCODE__OP_ERROR)
    {
        message_t__free_unpacked(response, NULL);
        return -1;
    }
    message_t__free_unpacked(response, NULL);
    return 0;
}

/* Devolve o número de elementos contidos na tabela.
 */
int rtable_size(struct rtable_t *rtable)
{

    if (rtable == NULL)
        return -1;

    MessageT msg;
    message_t__init(&msg);
    msg.opcode = MESSAGE_T__OPCODE__OP_SIZE;
    msg.c_type = MESSAGE_T__C_TYPE__CT_NONE;
    msg.data_size = 0;

    MessageT *response;
    if ((response = network_send_receive(rtable, &msg)) == NULL)
    {
        message_t__free_unpacked(response, NULL);
        return -1;
    }

    printf("Opcode:%d - c_type%d\n", response->opcode, response->c_type);
    if (response->opcode != MESSAGE_T__OPCODE__OP_SIZE + 1 || response->c_type != MESSAGE_T__C_TYPE__CT_RESULT || response->opcode == MESSAGE_T__OPCODE__OP_ERROR)
    {
        message_t__free_unpacked(response, NULL);
        return -1;
    }

    int size = response->data_size;
    message_t__free_unpacked(response, NULL);
    return size;
}

/* Devolve um array de char* com a cópia de todas as keys da tabela,
 * colocando um último elemento a NULL.
 */
char **rtable_get_keys(struct rtable_t *rtable)
{
    if (rtable == NULL)
        return NULL;

    MessageT msg;
    message_t__init(&msg);
    msg.opcode = MESSAGE_T__OPCODE__OP_GETKEYS;
    msg.c_type = MESSAGE_T__C_TYPE__CT_NONE;
    msg.data_size = 0;

    MessageT *response;
    if ((response = network_send_receive(rtable, &msg)) == NULL)
    {
        message_t__free_unpacked(response, NULL);
        return NULL;
    }

    printf("Opcode:%d - c_type%d\n", response->opcode, response->c_type);
    if (response->opcode != MESSAGE_T__OPCODE__OP_GETKEYS + 1 || response->c_type != MESSAGE_T__C_TYPE__CT_KEYS || response->opcode == MESSAGE_T__OPCODE__OP_ERROR)
    { //&& response->data == NULL
        message_t__free_unpacked(response, NULL);
        return NULL;
    }

    char **keysCopy = malloc(sizeof(char *) * (response->n_keys + 1));

    for (int i = 0; i < response->n_keys; i++)
    {
        keysCopy[i] = malloc(strlen(response->keys[i]) + 1);
        strcpy(keysCopy[i], response->keys[i]);
    }

    keysCopy[response->n_keys] = NULL;
    message_t__free_unpacked(response, NULL);

    return keysCopy;
}

/* Liberta a memória alocada por rtable_get_keys().
 */
void rtable_free_keys(char **keys)
{
    if (keys == NULL)
        return;
    int i = 0;
    while (keys[i] != NULL)
    {
        free(keys[i]);
        i++;
    }
    free(keys);
    keys = NULL;
}

/* Função que imprime o conteúdo da tabela remota para o terminal.
 */
void rtable_print(struct rtable_t *rtable)
{
    if (rtable == NULL)
        return;

    MessageT msg;
    message_t__init(&msg);
    msg.opcode = MESSAGE_T__OPCODE__OP_PRINT;
    msg.c_type = MESSAGE_T__C_TYPE__CT_NONE;
    msg.data_size = 0;

    MessageT *response;
    if ((response = network_send_receive(rtable, &msg)) == NULL)
    {
        message_t__free_unpacked(response, NULL);
        return;
    }

    printf("Opcode:%d - c_type%d\n", response->opcode, response->c_type);
    if (response->opcode != MESSAGE_T__OPCODE__OP_PRINT + 1 || response->c_type != MESSAGE_T__C_TYPE__CT_TABLE || response->opcode == MESSAGE_T__OPCODE__OP_ERROR)
    { //&& response->data == NULL
        message_t__free_unpacked(response, NULL);
        return;
    }
    for (int i = 0; i < response->n_entries; i++)
    {
        printf("%s=%s ", response->entries[i]->key, response->entries[i]->data.data);
    }
    message_t__free_unpacked(response, NULL);
}

struct statistics *rtable_stats(struct rtable_t *rtable)
{

    if (rtable == NULL)
        return NULL;

    MessageT msg;
    message_t__init(&msg);
    msg.opcode = MESSAGE_T__OPCODE__OP_STATS;
    msg.c_type = MESSAGE_T__C_TYPE__CT_NONE;

    MessageT *response;
    if ((response = network_send_receive(rtable, &msg)) == NULL)
    {
        message_t__free_unpacked(response, NULL);
        return NULL;
    }

    printf("Opcode:%d - c_type%d\n", response->opcode, response->c_type);
    if (response->opcode != MESSAGE_T__OPCODE__OP_STATS + 1 || response->c_type != MESSAGE_T__C_TYPE__CT_RESULT || response->opcode == MESSAGE_T__OPCODE__OP_ERROR)
    {
        message_t__free_unpacked(response, NULL);
        return NULL;
    }

    struct statistics *stats = malloc(sizeof(struct statistics));
    stats->currentAverage = response->stats->currentaverage;
    stats->n_requests = (long)response->stats->n_requests;
    stats->opDel = (long)response->stats->opdel;
    stats->opGet = (long)response->stats->opget;
    stats->opGetKeys = (long)response->stats->opgetkeys;
    stats->opPut = (long)response->stats->opput;
    stats->opSize = (long)response->stats->opsize;
    stats->opStats = (long)response->stats->opstats;
    stats->opTablePrint = (long)response->stats->optableprint;
    message_t__free_unpacked(response, NULL);
    return stats;
}