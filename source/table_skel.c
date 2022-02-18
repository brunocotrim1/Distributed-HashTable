/*Realizado por:
-João Ferraz, nº49420.
-Miguel Carvalho, nº54399;
-Bruno Cotrim, nº54406;
*/
#include "table_skel.h"
#include "network_server.h"
#include "network_client.h"
#include "client_stub-private.h"
#include "serialization.h"
#include "string.h"
#include <signal.h>
#include <stdio.h>
#include <pthread.h>
#include "stats.h"
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <fcntl.h>  // for open
#include <unistd.h> // for close
struct table_t *table = NULL;
struct statistics *estatisticas;
pthread_mutex_t statsLock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t statsCon = PTHREAD_COND_INITIALIZER;
int statsWriters = 0;
int statsReaders = 0;

pthread_mutex_t tableLock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t tableCon = PTHREAD_COND_INITIALIZER;
int tableWriters = 0;
int tableReaders = 0;

struct clusterINFO *clusterinformation;


// pthread_mutex_t readersLock = PTHREAD_MUTEX_INITIALIZER;
//int readCounter = 0;
/* Inicia o skeleton da tabela.
 * O main() do servidor deve chamar esta função antes de poder usar a
 * função invoke(). O parâmetro n_lists define o número de listas a
 * serem usadas pela tabela mantida no servidor.
 * Retorna 0 (OK) ou -1 (erro, por exemplo OUT OF MEMORY)
 */
int table_skel_init(int n_lists)
{
    table = table_create(n_lists);
    estatisticas = calloc(1, sizeof(struct statistics));
    return table == NULL ? -1 : 0;
}

/* Liberta toda a memória e recursos alocados pela função table_skel_init.
 */
void table_skel_destroy()
{
    free(estatisticas);
    if (table != NULL)
        table_destroy(table);
}

/* Executa uma operação na tabela (indicada pelo opcode contido em msg)
 * e utiliza a mesma estrutura message_t para devolver o resultado.
 * Retorna 0 (OK) ou -1 (erro, por exemplo, tabela nao incializada)
*/
int invoke(MessageT *msg)
{
    if (table == NULL || msg == NULL)
        return -1;
    printf("Query Received: %d %d\n", msg->opcode, msg->c_type);
    if (msg->opcode == MESSAGE_T__OPCODE__OP_GET && msg->c_type == MESSAGE_T__C_TYPE__CT_KEY)
    {
        msg->opcode = MESSAGE_T__OPCODE__OP_GET + 1;
        msg->c_type = MESSAGE_T__C_TYPE__CT_VALUE;
        enterCriticalRegion(&tableCon, &tableLock, READER, &tableWriters, &tableReaders);
        struct data_t *data = table_get(table, (char *)msg->data.data);
        leaveCriticalRegion(&tableCon, &tableLock, READER, &tableWriters, &tableReaders);
        if (data == NULL)
        {
            printf("DATA NOT FOUND!\n");
            free(msg->data.data);
            msg->data.data = NULL;
            msg->data.len = 0;
            return 0;
        }
        free(msg->data.data);
        msg->data.data = (uint8_t *)data->data;
        msg->data.len = data->datasize;
        free(data);
        return 0;
    }
    else if (msg->opcode == MESSAGE_T__OPCODE__OP_PUT && msg->c_type == MESSAGE_T__C_TYPE__CT_ENTRY)
    {
        if (msg->entries == NULL || msg->entries[0] == NULL || msg->n_entries != 1)
            return -1;
        if (clusterinformation->SERVERMODE == 1 && clusterinformation->BACKUPEXISTS == -1)
        {
            return -1;
        }
        struct data_t *temp = data_create2(msg->entries[0]->data.len, msg->entries[0]->data.data);
        enterCriticalRegion(&tableCon, &tableLock, WRITER, &tableWriters, &tableReaders);
        if (clusterinformation->SERVERMODE == PRIMARY)
        {
            struct data_t *temp1 = data_create2(msg->entries[0]->data.len, msg->entries[0]->data.data);
            struct entry_t *entry = entry_create(msg->entries[0]->key, temp1);

            if (putBackUp(entry) == -1)
            {
                leaveCriticalRegion(&tableCon, &tableLock, WRITER, &tableWriters, &tableReaders);
                printf("Error Copying Data to BACKUP\n");
                free(temp1);
                free(entry);
                return -1;
            }

            free(temp1);
            free(entry);
        }
        int status = table_put(table, msg->entries[0]->key, temp);
        leaveCriticalRegion(&tableCon, &tableLock, WRITER, &tableWriters, &tableReaders);
        if (status == -1)
        {
            printf("ERROR PUTTING\n");
            return -1;
        }
        free(temp);
        msg->data_size = 0;
        msg->opcode = MESSAGE_T__OPCODE__OP_PUT + 1;
        msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
        return 0;
    }
    else if (msg->opcode == MESSAGE_T__OPCODE__OP_DEL && msg->c_type == MESSAGE_T__C_TYPE__CT_KEY)

    {

        if (clusterinformation->SERVERMODE == 1 && clusterinformation->BACKUPEXISTS == -1)
        {
            return -1;
        }

        printf("DELETING!\n");
        enterCriticalRegion(&tableCon, &tableLock, WRITER, &tableWriters, &tableReaders);
        if (clusterinformation->SERVERMODE == PRIMARY && delBackUp((char *)msg->data.data) != 0)
        {
            leaveCriticalRegion(&tableCon, &tableLock, WRITER, &tableWriters, &tableReaders);
            printf("Error Deliting Data in BACKUP: Data Not Found\n");
            return -1;
        }
        int status = table_del(table, (char *)msg->data.data);
        leaveCriticalRegion(&tableCon, &tableLock, WRITER, &tableWriters, &tableReaders);

        if (status == -1)
            return -1;
        msg->data_size = 0;
        msg->opcode = MESSAGE_T__OPCODE__OP_DEL + 1;
        msg->c_type = MESSAGE_T__C_TYPE__CT_NONE;
        return 0;
    }

    else if (msg->opcode == MESSAGE_T__OPCODE__OP_SIZE && msg->c_type == MESSAGE_T__C_TYPE__CT_NONE)
    {

        printf("GETTING SIZE!\n");
        enterCriticalRegion(&tableCon, &tableLock, READER, &tableWriters, &tableReaders);
        int size = table_size(table);
        leaveCriticalRegion(&tableCon, &tableLock, READER, &tableWriters, &tableReaders);
        msg->data_size = size;
        msg->opcode = MESSAGE_T__OPCODE__OP_SIZE + 1;
        msg->c_type = MESSAGE_T__C_TYPE__CT_RESULT;
        return 0;
    }
    else if (msg->opcode == MESSAGE_T__OPCODE__OP_GETKEYS && msg->c_type == MESSAGE_T__C_TYPE__CT_NONE)
    {
        printf("GETTING KEYS!\n");
        int size = table_size(table);
        msg->opcode = MESSAGE_T__OPCODE__OP_GETKEYS + 1;
        msg->c_type = MESSAGE_T__C_TYPE__CT_KEYS;
        enterCriticalRegion(&tableCon, &tableLock, READER, &tableWriters, &tableReaders);
        char **keys = table_get_keys(table);
        leaveCriticalRegion(&tableCon, &tableLock, READER, &tableWriters, &tableReaders);

        if (keys == NULL)

            return -1;

        msg->keys = keys;
        msg->n_keys = size;

        return 0;
    }

    else if (msg->opcode == MESSAGE_T__OPCODE__OP_PRINT && msg->c_type == MESSAGE_T__C_TYPE__CT_NONE)
    {
        printf("GETTING PRINT!\n");
        msg->n_entries = table_size(table);
        msg->entries = malloc(msg->n_entries * sizeof(MessageT__Entry *));
        enterCriticalRegion(&tableCon, &tableLock, READER, &tableWriters, &tableReaders);
        char **keys = table_get_keys(table);
        if (keys == NULL)
            return -1;

        int i = 0;
        while (keys[i] != NULL)
        {
            msg->entries[i] = malloc(sizeof(MessageT__Entry));
            message_t__entry__init(msg->entries[i]);
            msg->entries[i]->key = malloc(strlen(keys[i]) + 1);
            strcpy(msg->entries[i]->key, keys[i]);
            struct data_t *temp = table_get(table, keys[i]);
            msg->entries[i]->data.data = temp->data;
            msg->entries[i]->data.len = temp->datasize;
            free(temp);
            i++;
        }
        leaveCriticalRegion(&tableCon, &tableLock, READER, &tableWriters, &tableReaders);
        table_free_keys(keys);

        msg->opcode = MESSAGE_T__OPCODE__OP_PRINT + 1;
        msg->c_type = MESSAGE_T__C_TYPE__CT_TABLE;
        return 0;
    }
    else if (msg->opcode == MESSAGE_T__OPCODE__OP_STATS && msg->c_type == MESSAGE_T__C_TYPE__CT_NONE)
    {
        msg->stats = malloc(sizeof(MessageT__Stats));
        message_t__stats__init(msg->stats);
        enterCriticalRegion(&statsCon, &statsLock, READER, &statsWriters, &statsReaders);
        msg->stats->currentaverage = estatisticas->currentAverage;
        msg->stats->n_requests = estatisticas->n_requests;
        msg->stats->opdel = estatisticas->opDel;
        msg->stats->opget = estatisticas->opGet;
        msg->stats->opgetkeys = estatisticas->opGetKeys; //REGIÃO CRÍTICA
        msg->stats->opput = estatisticas->opPut;
        msg->stats->opsize = estatisticas->opSize;
        msg->stats->opstats = estatisticas->opStats;
        msg->stats->optableprint = estatisticas->opTablePrint;
        leaveCriticalRegion(&statsCon, &statsLock, READER, &statsWriters, &statsReaders);
        msg->opcode = MESSAGE_T__OPCODE__OP_STATS + 1;
        msg->c_type = MESSAGE_T__C_TYPE__CT_RESULT;
        return 0;
    }

    msg->data_size = 0;
    msg->opcode = MESSAGE_T__OPCODE__OP_BAD;
    msg->c_type = MESSAGE_T__C_TYPE__CT_BAD;

    return 0;
}

void updateStats(int opcode, double timeSec)
{
    enterCriticalRegion(&statsCon, &statsLock, WRITER, &statsWriters, &statsReaders);

    if (opcode != MESSAGE_T__OPCODE__OP_DEL && opcode != MESSAGE_T__OPCODE__OP_GET && opcode != MESSAGE_T__OPCODE__OP_SIZE && opcode != MESSAGE_T__OPCODE__OP_PUT && opcode != MESSAGE_T__OPCODE__OP_GETKEYS && opcode != MESSAGE_T__OPCODE__OP_PRINT && opcode != MESSAGE_T__OPCODE__OP_STATS)
        return;
    if (estatisticas->n_requests == 0 && estatisticas->currentAverage == 0.0)
    {
        estatisticas->currentAverage = timeSec;
    }
    else
    {
        double lastSum = estatisticas->currentAverage * estatisticas->n_requests;
        lastSum += timeSec;
        estatisticas->currentAverage = lastSum / (double)(estatisticas->n_requests + 1);
    }
    estatisticas->n_requests++;
    switch (opcode)
    {
    case MESSAGE_T__OPCODE__OP_DEL:
        estatisticas->opDel++;
        break;
    case MESSAGE_T__OPCODE__OP_GET:
        estatisticas->opGet++;
        break;
    case MESSAGE_T__OPCODE__OP_SIZE:
        estatisticas->opSize++;
        break;
    case MESSAGE_T__OPCODE__OP_PUT:
        estatisticas->opPut++;
        break;
    case MESSAGE_T__OPCODE__OP_GETKEYS:
        estatisticas->opGetKeys++;
        break;
    case MESSAGE_T__OPCODE__OP_PRINT:
        estatisticas->opTablePrint++;
        break;
    case MESSAGE_T__OPCODE__OP_STATS:
        estatisticas->opStats++;
        break;
    default:
        break;
    }
    leaveCriticalRegion(&statsCon, &statsLock, WRITER, &statsWriters, &statsReaders);
    printf("Request took %f seconds to execute \n", timeSec);
}

void enterCriticalRegion(pthread_cond_t *con, pthread_mutex_t *mutex, int action, int *writerCounter, int *readerCounter)
{
    pthread_mutex_lock(mutex);
    if (action == READER) //No caso de ser um Reader este não pode entrar se um writer estiver a escrever, no entanto se nenhum estiver a escrever poderão entrar n readers, caso contrario ficará bloqueado à espera de uma ordem de saida
    {
        while (*writerCounter == 1)
            pthread_cond_wait(con, mutex);
        (*readerCounter)++;
    }
    else if (action == WRITER) // se for um writer e outro writer estiver a escrever ou leitores estiverem a ler este fica à espera de ordem para proseguir verificando sempre se as condições de entrada se verificam
    {
        while (*writerCounter == 1 || *readerCounter > 0)
            pthread_cond_wait(con, mutex);
        (*writerCounter)++;
    }
    pthread_mutex_unlock(mutex);
}
void leaveCriticalRegion(pthread_cond_t *con, pthread_mutex_t *mutex, int action, int *writerCounter, int *readerCounter)
{
    pthread_mutex_lock(mutex);

    if (action == READER)
    {
        (*readerCounter)--;
    }
    else if (action == WRITER)
    {
        (*writerCounter)--;
    }
    pthread_cond_broadcast(con);
    pthread_mutex_unlock(mutex);
}

void setClusterInformation(struct clusterINFO *clusterinformationP)
{
    clusterinformation = clusterinformationP;
}

int sendAllDataToBackUp()
{
    int status = 0;
    if (table_size(table) == 0)
    {
        printf("No Data to transfer to BackUp\n");
        return 0;
    }
    enterCriticalRegion(&tableCon, &tableLock, READER, &tableWriters, &tableReaders);
    char **keys = table_get_keys(table);
    if (keys == NULL)
        return -1;

    int i = 0;
    while (keys[i] != NULL)
    {
        struct data_t *temp = table_get(table, keys[i]);
        char *keyCpy = malloc(sizeof(keys[i]) + 1);
        strcpy(keyCpy, keys[i]);

        struct entry_t *entry = entry_create(keyCpy, temp);
        if (putBackUp(entry) == -1)
            status = -1;
        entry_destroy(entry);
        i++;
    }
    table_free_keys(keys);
    leaveCriticalRegion(&tableCon, &tableLock, READER, &tableWriters, &tableReaders);
    return status;
}

int putBackUp(struct entry_t *entry)
{
    if (entry == NULL)
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
    struct rtable_t *rtable = calloc(1, sizeof(struct rtable_t));
    rtable->socketfd = clusterinformation->BACKUPSOCKET;
    if ((response = network_send_receive(rtable, &msg)) == NULL)
    {
        message_t__free_unpacked(response, NULL);
        free(msg.keys);
        free(msg.entries[0]);
        free(msg.entries);
        free(rtable->adress);

        free(rtable);
        return -1;
    }
    int status = response->opcode;
    printf("Copied to BACKUP, Received:%d\n", status);
    message_t__free_unpacked(response, NULL);
    rtable->adress = NULL;
    free(msg.keys);
    free(msg.entries[0]);
    free(msg.entries);
    free(rtable->adress);
    free(rtable);
    if (status != MESSAGE_T__OPCODE__OP_PUT + 1)
    {
        return -1;
    }
    return 0;
}
int delBackUp(char *key)
{

    if (key == NULL)
        return -1;

    MessageT msg;
    message_t__init(&msg);
    msg.opcode = MESSAGE_T__OPCODE__OP_DEL;
    msg.c_type = MESSAGE_T__C_TYPE__CT_KEY;
    msg.data.data = key;
    msg.data.len = strlen(key) + 1;
    MessageT *response;
    struct rtable_t *rtable = calloc(1, sizeof(struct rtable_t));
    rtable->socketfd = clusterinformation->BACKUPSOCKET;
    rtable->adress = NULL;
    if ((response = network_send_receive(rtable, &msg)) == NULL)
    {
        message_t__free_unpacked(response, NULL);
        free(rtable->adress);
        free(rtable);
        return -1;
    }
    if (response->opcode != MESSAGE_T__OPCODE__OP_DEL + 1 || response->c_type != MESSAGE_T__C_TYPE__CT_NONE || response->opcode == MESSAGE_T__OPCODE__OP_ERROR)
    {
        message_t__free_unpacked(response, NULL);
        free(rtable->adress);
        free(rtable);
        return -1;
    }
    free(rtable->adress);
    free(rtable);
    message_t__free_unpacked(response, NULL);
    return 0;
}
