#ifndef _TABLE_SKEL_H
#define _TABLE_SKEL_H

#include "sdmessage.pb-c.h"
#include "table.h"
#include "entry.h"
#include <stdlib.h>
#include "message-private.h"
#define READER 1
#define WRITER 0
struct clusterINFO
{
    int BACKUPEXISTS;
    int BACKUPSOCKET;
    int SERVERMODE;
};

/* Inicia o skeleton da tabela.
 * O main() do servidor deve chamar esta função antes de poder usar a
 * função invoke(). O parâmetro n_lists define o número de listas a
 * serem usadas pela tabela mantida no servidor.
 * Retorna 0 (OK) ou -1 (erro, por exemplo OUT OF MEMORY)
 */
int table_skel_init(int n_lists);

/* Liberta toda a memória e recursos alocados pela função table_skel_init.
 */
void table_skel_destroy();

/* Executa uma operação na tabela (indicada pelo opcode contido em msg)
 * e utiliza a mesma estrutura message_t para devolver o resultado.
 * Retorna 0 (OK) ou -1 (erro, por exemplo, tabela nao incializada)
*/
int invoke(MessageT *msg);

void updateStats(int opcode, double timeSec);
void enterCriticalRegion(pthread_cond_t *con, pthread_mutex_t *mutex, int action, int *writerCounter, int *readerCounter);
void leaveCriticalRegion(pthread_cond_t *con, pthread_mutex_t *mutex, int action, int *writerCounter, int *readerCounter);
void setClusterInformation(struct clusterINFO *clusterinformationP);
int connectToBackUp(char *adress_port);
int putBackUp(struct entry_t *entry);
int delBackUp(char *key);
#endif
