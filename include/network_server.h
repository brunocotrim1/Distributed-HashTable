#ifndef _NETWORK_SERVER_H
#define _NETWORK_SERVER_H

#include "table_skel.h"
#include "zookeeper/zookeeper.h"
#define PRIMARY 1
#define BACKUP 0
/* Função para preparar uma socket de receção de pedidos de ligação
 * num determinado porto.
 * Retornar descritor do socket (OK) ou -1 (erro).
 */
int network_server_init(short port, const char *ZHaddress_port);

/* Esta função deve:
 * - Aceitar uma conexão de um cliente;
 * - Receber uma mensagem usando a função network_receive;
 * - Entregar a mensagem de-serializada ao skeleton para ser processada;
 * - Esperar a resposta do skeleton;
 * - Enviar a resposta ao cliente usando a função network_send.
 */
int network_main_loop(int listening_socket);

/* Esta função deve:
 * - Ler os bytes da rede, a partir do client_socket indicado;
 * - De-serializar estes bytes e construir a mensagem com o pedido,
 *   reservando a memória necessária para a estrutura message_t.
 */
MessageT *network_receive(int client_socket);

/* Esta função deve:
 * - Serializar a mensagem de resposta contida em msg;
 * - Libertar a memória ocupada por esta mensagem;
 * - Enviar a mensagem serializada, através do client_socket.
 */
int network_send(int client_socket, MessageT *msg);

/* A função network_server_close() liberta os recursos alocados por
 * network_server_init().
 */
int network_server_close();

void *connectionHandler(void *clientSocket);
int ConnectToZookeeper(const char *address_port);
void connection_watcher(zhandle_t *zzh, int type, int state, const char *path, void *context);
int ZookeeperSetup(const char *ZHaddress_port);
static void watcher_callback(zhandle_t *wzh, int type, int state, const char *zpath, void *watcher_ctx);
int sendAllDataToBackUp();
int network_send_backup();
#endif
