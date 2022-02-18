/*Realizado por:
-João Ferraz, nº49420.
-Miguel Carvalho, nº54399;
-Bruno Cotrim, nº54406;
*/
#include "message-private.h"
#include <sys/types.h>
#include <stdio.h>
int send_all(int socketfd, uint8_t *buffer, int length)
{
    char *ptr = (char *)buffer;
    int total = 0;
    int bytesleft = length;
    int n;
    while (total < length)
    {
        {
            n = send(socketfd, buffer + total, bytesleft, 0);
            if (n == -1)
            {
                break;
            }
            total += n;

            bytesleft -= n;
        }
    }
    return n == -1 ? -1 : total; //Retorna a totalidade dos bytes enviados
}
int receive_all(int socketfd, uint8_t *buffer, unsigned len)
{
    char *ptr = (char *)buffer;
    int total = 0;
    int bytesleft = len;
    int n;
    while (total < len)
    {
        {
            n = recv(socketfd, buffer + total, bytesleft, 0);
            if (n == -1 || n == 0)
            {
                n = -1;
                break;
            }
            total += n;

            bytesleft -= n;
        }
    }
    return n == -1 ? -1 : total; //Retorna a totalidade dos bytes enviados
}