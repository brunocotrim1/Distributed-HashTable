/*Realizado por:
-João Ferraz, nº49420.
-Miguel Carvalho, nº54399;
-Bruno Cotrim, nº54406;
*/
#include "network_client.h"
#include "client_stub-private.h"
#include "message-private.h"
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

/* Esta função deve:
 * - Obter o endereço do servidor (struct sockaddr_in) a base da
 *   informação guardada na estrutura rtable;
 * - Estabelecer a ligação com o servidor;
 * - Guardar toda a informação necessária (e.g., descritor do socket)
 *   na estrutura rtable;
 * - Retornar 0 (OK) ou -1 (erro).
 */
int network_connect(struct rtable_t *rtable)
{
    if (rtable == NULL)
        return -1;
    int socketfd;
    struct sockaddr_in server;

    if ((socketfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("Erro ao criar socket TCP");
        return -1;
    }

    server.sin_family = AF_INET;
    server.sin_port = htons(rtable->port);

    if (inet_pton(AF_INET, rtable->adress, &server.sin_addr) < 1)
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

    rtable->socketfd = socketfd;
    return 0;
}

/* Esta função deve:
 * - Obter o descritor da ligação (socket) da estrutura rtable_t;
 * - Reservar memória para serializar a mensagem contida em msg;
 * - Serializar a mensagem contida em msg;
 * - Enviar a mensagem serializada para o servidor;
 * - Libertar a memória ocupada pela mensagem serializada enviada;
 * - Esperar a resposta do servidor;
 * - Reservar a memória para a mensagem serializada recebida;
 * - De-serializar a mensagem de resposta, reservando a memória 
 *   necessária para a estrutura message_t que é devolvida;
 * - Libertar a memória ocupada pela mensagem serializada recebida;
 * - Retornar a mensagem de-serializada ou NULL em caso de erro.
 */
MessageT *network_send_receive(struct rtable_t *rtable,
                               MessageT *msg)
{

    if (rtable == NULL || msg == NULL)
        return NULL;
    int socketfd = rtable->socketfd;
    unsigned len;
    len = message_t__get_packed_size(msg);
    uint8_t *buf = malloc(len);
    if (buf == NULL)
    {
        fprintf(stdout, "malloc error\n");
        close(socketfd);
        return NULL;
    }

    message_t__pack(msg, buf);
    int nbytes;

    uint8_t *buf1 = malloc(len);
    unsigned lenNet = htonl(len);
    memcpy(buf1, &lenNet, sizeof(unsigned));
    if ((nbytes = send_all(rtable->socketfd, buf1, sizeof(unsigned))) == -1)
    {
        perror("Erro ao enviar dados ao servidor");
        close(rtable->socketfd);
        free(buf);
        free(buf1);
        return 0;
    }

    if ((nbytes = send_all(socketfd, buf, len)) == -1)
    {
        perror("Erro ao enviar dados ao servidor");
        close(socketfd);
        free(buf);
        free(buf1);
        return NULL;
    }
    free(buf);
    free(buf1);


    uint8_t *response = malloc(MAX_MSG);
    unsigned *msgLen = malloc(sizeof(unsigned));
    if ((nbytes = receive_all(socketfd, (uint8_t *)msgLen, sizeof(unsigned))) == -1)
    {
        perror("Erro ao receber dados do servidor");
        close(socketfd);
        free(response);
        free(msgLen);
        return NULL;
    };
    *msgLen = ntohl(*msgLen);
    if (*msgLen > 0 && (nbytes = receive_all(socketfd, response, *msgLen)) == -1)
    {
        perror("Erro ao receber dados do servidor");
        close(socketfd);
        free(response);
        free(msgLen);
        return NULL;
    };
    MessageT *recv_msg = message_t__unpack(NULL, nbytes, response);
    free(response);
    if (recv_msg == NULL)
    {
        message_t__free_unpacked(recv_msg, NULL);
        fprintf(stdout, "error unpacking message\n");
        close(socketfd);
        free(msgLen);
        return NULL;
    }
    free(msgLen);
    return recv_msg;
}

/* A função network_close() fecha a ligação estabelecida por
 * network_connect().
 */
int network_close(struct rtable_t *rtable)
{
    return close(rtable->socketfd);
}
