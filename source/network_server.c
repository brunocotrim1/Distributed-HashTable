/*Realizado por:
-João Ferraz, nº49420.
-Miguel Carvalho, nº54399;
-Bruno Cotrim, nº54406;
*/
#include "network_server.h"
#include <sys/socket.h>
#include <errno.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include "message-private.h"
#include "serialization.h"
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <signal.h>
#include <time.h>
#include <sys/time.h>

/* Função para preparar uma socket de receção de pedidos de ligação
 * num determinado porto.
 * Retornar descritor do socket (OK) ou -1 (erro).
 */
static zhandle_t *zh;
typedef struct String_vector zoo_string;
static int is_connected;
char *addr_port;
int serverSocket;
struct clusterINFO *clusterinformation;
int primarySocket;
int ZDATALEN = 1024 * 1024;
int network_server_close()
{
    printf("Interrupted:Closing Socket\n");
    table_skel_destroy();
    close(serverSocket);
    zookeeper_close(zh);
    free(addr_port);
    if (clusterinformation->BACKUPSOCKET >= 0)
        close(clusterinformation->BACKUPSOCKET);
    free(clusterinformation);
    exit(-1);
}

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

static void watcher_callback(zhandle_t *wzh, int type, int state, const char *zpath, void *watcher_ctx)
{
    if (state == ZOO_CONNECTED_STATE)
    {
        if (type == ZOO_CHILD_EVENT)
        {
            int existsKvPrimary = zoo_exists(zh, "/kvstore/primary", 0, NULL);
            int existsKvBackup = zoo_exists(zh, "/kvstore/backup", 0, NULL);
            if (clusterinformation->SERVERMODE == PRIMARY && existsKvBackup == ZNONODE)
            {
                clusterinformation->BACKUPEXISTS = -1;
                clusterinformation->BACKUPSOCKET = -1;
                printf("BACKUPSERVER DIED!\n");
            }
            else if (clusterinformation->SERVERMODE == BACKUP && existsKvPrimary == ZNONODE)
            {
                printf("PRIMARY SERVER DIED, TAKING CONTROL!\n");
                if (ZOK != zoo_create(zh, "/kvstore/primary", addr_port, strlen(addr_port), &ZOO_OPEN_ACL_UNSAFE, ZOO_EPHEMERAL, NULL, 0))
                    fprintf(stderr, "Error Creating Node /kvstore/primary!\n");
                if (ZOK == zoo_delete(zh, "/kvstore/backup", -1))
                {
                    printf("NODE /kvstore/backup Deleted!\n");
                }
                primarySocket = -1;
                clusterinformation->SERVERMODE = PRIMARY;
                clusterinformation->BACKUPEXISTS = -1;
            }
            else if (clusterinformation->SERVERMODE == PRIMARY && existsKvBackup == ZOK && clusterinformation->BACKUPEXISTS == -1)
            {
                char *zdata_buf = (char *)calloc(1, ZDATALEN * sizeof(char));
                if (ZOK != zoo_get(zh, "/kvstore/backup", 0, zdata_buf, &ZDATALEN, NULL))
                {
                    printf("ERROR GETTING BACKUP INFO\n");
                }
                printf("BACKUP INFO = %s\n", zdata_buf);
                if ((clusterinformation->BACKUPSOCKET = connectToBackUp(zdata_buf)) != -1)
                {
                    printf("Connected To BackUp Suceffuly in SOCKET %d\n", clusterinformation->BACKUPSOCKET);
                }
                if (sendAllDataToBackUp() == 0)
                {
                    printf("Copy all Data to BACKUP Sucefful\n");
                    clusterinformation->BACKUPEXISTS = 1;
                }
                else
                    printf("Error Copying All Data to Backup\n");
                free(zdata_buf);
            }
            if (ZOK != zoo_wget_children(zh, "/kvstore", watcher_callback, "ZooKeeper Data Watcher", NULL))
            {
                fprintf(stderr, "Error setting watch at /kvstore - CB!\n");
            }
        }
    }
}

int connectToBackUp(char *adress_port)
{
    struct sockaddr_in server;
    char *token = strtok(adress_port, ":");

    if (token == NULL)
        return -1;

    char *ipAdress = token;

    token = strtok(NULL, ":");

    short port = atoi(token);

    if (token == NULL || port == 0)
        return -1;

    int socketfd;
    if ((socketfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("Erro ao criar socket TCP");
        return -1;
    }

    server.sin_family = AF_INET;
    server.sin_port = htons(port);

    if (inet_pton(AF_INET, ipAdress, &server.sin_addr) < 1)
    {
        printf("Erro ao converter IP\n");
        close(socketfd);
        return -1;
    }

    if (connect(socketfd, (struct sockaddr *)&server, sizeof(server)) < 0)
    {
        perror("Erro ao conectar-se ao servidor");
        close(socketfd);
        return -1;
    }

    return socketfd;
}

int ConnectToZookeeper(const char *address_port)
{
    zh = zookeeper_init(address_port, connection_watcher, 2000, 0, NULL, 0);
    if (zh == NULL)
    {
        fprintf(stderr, "Error connecting to ZooKeeper server!\n");
        return -1;
    }
    return 0;
}

int ZookeeperSetup(const char *ZHaddress_port)
{
    while (1)
    {
        if (is_connected == 1)
        {
            int existsKvStore = zoo_exists(zh, "/kvstore", 0, NULL);
            int existsKvPrimary = zoo_exists(zh, "/kvstore/primary", 0, NULL);
            int existsKvBackup = zoo_exists(zh, "/kvstore/backup", 0, NULL);
            if (ZNONODE == existsKvStore)
            {
                if (ZOK != zoo_create(zh, "/kvstore", NULL, -1, &ZOO_OPEN_ACL_UNSAFE, 0, NULL, 0))
                    fprintf(stderr, "Error Creating Node /kvstore!\n");
                else
                {
                    printf("Node /kvstore Created!\n");
                }
                if (ZOK != zoo_create(zh, "/kvstore/primary", ZHaddress_port, strlen(ZHaddress_port), &ZOO_OPEN_ACL_UNSAFE, ZOO_EPHEMERAL, NULL, 0))
                    fprintf(stderr, "Error Creating Node /kvstore/primary!\n");
                else
                {
                    printf("Node /kvstore/primary Created!\n");
                    clusterinformation->SERVERMODE = PRIMARY;
                    clusterinformation->BACKUPEXISTS = -1;
                }
                break;
            }
            else if (existsKvStore == ZOK && existsKvPrimary == ZOK && existsKvBackup == ZOK)
            {
                return -1;
            }
            else if (existsKvPrimary == ZNONODE && existsKvBackup == ZNONODE)
            {
                if (ZOK != zoo_create(zh, "/kvstore/primary", ZHaddress_port, strlen(ZHaddress_port), &ZOO_OPEN_ACL_UNSAFE, ZOO_EPHEMERAL, NULL, 0))
                    fprintf(stderr, "Error Creating Node /kvstore/primary!\n");
                else
                {
                    printf("Node /kvstore/primary Created!\n");
                    clusterinformation->SERVERMODE = PRIMARY;
                    clusterinformation->BACKUPEXISTS = -1;
                }
                break;
            }
            else if (existsKvPrimary == ZOK && existsKvBackup == ZNONODE)
            {
                if (ZOK != zoo_create(zh, "/kvstore/backup", ZHaddress_port, strlen(ZHaddress_port), &ZOO_OPEN_ACL_UNSAFE, ZOO_EPHEMERAL, NULL, 0))
                    fprintf(stderr, "Error Creating Node /kvstore/backup!\n");
                else
                {
                    printf("Node /kvstore/backup Created!\n");
                    clusterinformation->SERVERMODE = BACKUP;
                    clusterinformation->BACKUPEXISTS = -1;
                    struct sockaddr_in clientT;
                    socklen_t size_clientT;
                    primarySocket = accept(serverSocket, (struct sockaddr *)&clientT, &size_clientT);
                    if (primarySocket <= 0)
                        return -1;
                    pthread_t thread;
                    pthread_attr_t threadAttributes;
                    int status = pthread_attr_init(&threadAttributes);
                    status = pthread_attr_setdetachstate(&threadAttributes, PTHREAD_CREATE_DETACHED);
                    int *socketCopy = malloc(sizeof(int));
                    *socketCopy = primarySocket;
                    if (status == 0 && clusterinformation->SERVERMODE == BACKUP)
                    {
                        pthread_create(&thread, &threadAttributes, connectionHandler, socketCopy);
                        printf("Connected to Primary\n");
                    }
                    else
                    {
                        printf("Client Tried To Connect To an existing Backup Server\n");
                        close(*socketCopy);
                        free(socketCopy);
                    }
                    pthread_attr_destroy(&threadAttributes);
                }
                break;
            }
            else if (existsKvPrimary == ZNONODE && existsKvBackup == ZOK)
            {
                printf("Exists BackUp but does not exist Primary\n");
                sleep(5);
            }
        }
        else
            return -1;
    }
    if (ZOK != zoo_wget_children(zh, "/kvstore", &watcher_callback, "ZooKeeper Data Watcher", NULL))
    {
        fprintf(stderr, "Error setting watch at /kvstore!\n");
    }
    return 0;
}

int network_server_init(short port, const char *ZHaddress_port)
{

    int sockfd;
    struct sockaddr_in server;
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("Erro ao criar socket");
        return -1;
    }
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0)
        perror("setsockopt(SO_REUSEADDR) failed");
    server.sin_family = AF_INET;

    server.sin_port = htons(port);

    server.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(sockfd, (struct sockaddr *)&server, sizeof(server)) < 0)
    {
        perror("Erro ao fazer bind");
        close(sockfd);
        return -1;
    }

    if (listen(sockfd, 0) < 0)
    {
        perror("Erro ao executar listen");
        close(sockfd);
        return -1;
    }

    printf("Server Listening to Requests!\n");
    serverSocket = sockfd;
    clusterinformation = calloc(1, sizeof(struct clusterINFO));
    if (ConnectToZookeeper(ZHaddress_port) != 0)
        return -1;
    sleep(3);
    char portStr[124];
    sprintf(portStr, "%d", port);
    char *address = inet_ntoa(server.sin_addr);
    addr_port = calloc(1, strlen(address) + strlen(portStr) + 2);
    strcat(addr_port, address);
    strcat(addr_port, ":");
    strcat(addr_port, portStr);
    if (ZookeeperSetup(addr_port) != 0)
        return -1;
    if (port <= 0)
        return -1;
    setClusterInformation(clusterinformation);
    return sockfd;
}

/* Esta função deve:
 * - Aceitar uma conexão de um cliente;
 * - Receber uma mensagem usando a função network_receive;
 * - Entregar a mensagem de-serializada ao skeleton para ser processada;
 * - Esperar a resposta do skeleton;
 * - Enviar a resposta ao cliente usando a função network_send.
 */
int network_main_loop(int listening_socket)
{
    signal(SIGINT, (sig_t)network_server_close);
    signal(SIGSEGV, (sig_t)network_server_close);
    signal(SIGTSTP, (sig_t)network_server_close);
    signal(SIGABRT, (sig_t)network_server_close);
    signal(SIGPIPE, SIG_IGN);
    int connsockfd;
    struct sockaddr_in client;
    socklen_t size_client = sizeof((struct sockaddr *)&client);
    while ((connsockfd = accept(listening_socket, (struct sockaddr *)&client, &size_client)) >= 0)
    {
        if (clusterinformation->SERVERMODE == PRIMARY && primarySocket <= 0)
        {
            int quit = 0;
            char clientADDR[100];
            printf("\nConnected To a Client - %s:%d!\n", inet_ntop(AF_INET, &client.sin_addr, clientADDR, sizeof(clientADDR)), htons(client.sin_port));
            int *socketCopy = malloc(sizeof(int));
            *socketCopy = connsockfd;
            pthread_t thread;
            pthread_attr_t threadAttributes;
            int status;
            status = pthread_attr_init(&threadAttributes);
            status = pthread_attr_setdetachstate(&threadAttributes, PTHREAD_CREATE_DETACHED);
            if (status == 0)
                pthread_create(&thread, &threadAttributes, connectionHandler, socketCopy);
            else
            {

                close(*socketCopy);
                free(socketCopy);
            }
            pthread_attr_destroy(&threadAttributes);
        }
        else if (clusterinformation->SERVERMODE == BACKUP)
        {
            close(connsockfd);
            printf("Client Tried To Connect To an existing Backup Server\n");
        }
    }
    fprintf(stderr, "errno = %d\n", errno);
    printf("ERROR OCCURRED LEAVING LOOP - %d", connsockfd);
    return 0;
}

/* Esta função deve:
 * - Serializar a mensagem de resposta contida em msg;
 * - Libertar a memória ocupada por esta mensagem;
 * - Enviar a mensagem serializada, através do client_socket.
 */
int network_send(int client_socket, MessageT *msg)
{
    unsigned len;
    len = message_t__get_packed_size(msg);
    uint8_t *buf = malloc(len);
    if (buf == NULL)
    {
        fprintf(stdout, "malloc error\n");
        free(buf);
        close(client_socket);
        return -1;
    }
    message_t__pack(msg, buf);

    int nbytes;
    uint8_t *buf1 = malloc(len);
    unsigned lenNet = htonl(len);
    memcpy(buf1, &lenNet, sizeof(unsigned));
    if ((nbytes = send_all(client_socket, buf1, sizeof(unsigned))) == -1)
    {
        perror("Erro ao enviar dados ao cliente");
        close(client_socket);
        message_t__free_unpacked(msg, NULL);
        free(buf);
        free(buf1);
        return -1;
    }
    if ((nbytes = send_all(client_socket, buf, len)) == -1)
    {
        perror("Erro ao enviar dados ao cliente");
        close(client_socket);
        message_t__free_unpacked(msg, NULL);
        free(buf);
        free(buf1);
        return -1;
    }

    message_t__free_unpacked(msg, NULL);

    free(buf);
    free(buf1);
    return 0;
}

/* Esta função deve:
 * - Ler os bytes da rede, a partir do client_socket indicado;
 * - De-serializar estes bytes e construir a mensagem com o pedido,
 *   reservando a memória necessária para a estrutura message_t.
 */
MessageT *network_receive(int client_socket)
{
    int nbytes;

    unsigned msgLen = 0;
    if ((nbytes = receive_all(client_socket, (uint8_t *)&msgLen, sizeof(unsigned))) == -1)
    {
        perror("Erro ao receber dados do cliente");
        close(client_socket);
        return NULL;
    };
    msgLen = ntohl(msgLen);
    uint8_t *response = malloc(msgLen);
    if (msgLen <= 0 || (nbytes = receive_all(client_socket, response, msgLen)) == -1)
    {
        perror("Erro ao receber dados do cliente");
        free(response);

        return NULL;
    };
    MessageT *recv_msg = message_t__unpack(NULL, nbytes, response);
    if (recv_msg == NULL)
    {
        message_t__free_unpacked(recv_msg, NULL);
        fprintf(stdout, "error unpacking message\n");
        return NULL;
        free(response);
    }
    free(response);
    return recv_msg;
}

void *connectionHandler(void *clientSocket)
{

    int socket = *((int *)clientSocket);
    free(clientSocket);
    int quit = 0;
    while (quit == 0)
    {
        MessageT *clientRequest = network_receive(socket);
        struct timeval t1, t2;
        double time_taken;
        gettimeofday(&t1, NULL);
        if (clientRequest != NULL)
        {
            int responseOpcode = clientRequest->opcode;
            int status = invoke(clientRequest);
            printf("Status:%d\n", status);

            if (status == -1)
            {

                clientRequest->data_size = 0;
                clientRequest->opcode = MESSAGE_T__OPCODE__OP_ERROR;
                clientRequest->c_type = MESSAGE_T__C_TYPE__CT_NONE;
                clientRequest->data_size = -1;
                printf("ERROR OCURRED!\n");
                gettimeofday(&t2, NULL);
                time_taken = (t2.tv_sec - t1.tv_sec) * 1000.0;
                time_taken += (t2.tv_usec - t1.tv_usec) / 1000.0;
                updateStats(responseOpcode, time_taken);
                if (network_send(socket, clientRequest) == -1)
                    quit = 1;
            }
            else
            {
                gettimeofday(&t2, NULL);
                time_taken = (t2.tv_sec - t1.tv_sec) * 1000.0;
                time_taken += (t2.tv_usec - t1.tv_usec) / 1000.0;

                updateStats(responseOpcode, time_taken);
                if (network_send(socket, clientRequest) == -1)
                    quit = 1;
            }
        }
        else
            quit = 1;
    }
    close(socket);
}