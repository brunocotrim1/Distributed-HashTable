/*Realizado por:
-João Ferraz, nº49420.
-Miguel Carvalho, nº54399;
-Bruno Cotrim, nº54406;
*/
#include "client_stub.h"
#include "stats.h"
#include <stdio.h>
#include "client_stub-private.h"
#include <string.h>
#include <stdlib.h>
#include <signal.h>
char *address_prot = NULL;
struct rtable_t *tableRemote = NULL;
int sigHandler()
{
    rtable_disconnect(tableRemote);
    free(address_prot);
    exit(-1);
}
int main(int argc, char **argv)
{
    signal(SIGINT, (sig_t)sigHandler);
    signal(SIGPIPE, (sig_t)sigHandler);
    if (argc != 2)
    {
        printf("Número de argumentos errado, insira: ./table_client <address>:<port>");
    }
    signal(SIGPIPE, SIG_IGN);
    address_prot = malloc(100);
    strcpy(address_prot, argv[1]);
    char line[2048];
    tableRemote = rtable_connect(address_prot);
    if (tableRemote == NULL)
    {
        free(address_prot);
        rtable_disconnect(tableRemote);
        exit(-1);
    }
    while (1)
    {
        if (fgets(line, 2048, stdin) != NULL)
        {
            char *lineCpy = malloc(strlen(line) + 1);
            strcpy(lineCpy, line);
            if (strlen(lineCpy) >= 1)
                lineCpy[strlen(lineCpy) - 1] = '\0';
            char *token = strtok(lineCpy, " ");
            if (tableRemote->socketfd == -1)
            {
                printf("Not Connected to a Primary Server, unable to make operations\n");
            }
            else if (token != NULL && strcmp(token, "put") == 0)
            {
                token = strtok(NULL, " ");
                if (token != NULL)
                {
                    char *key = malloc(strlen(token) + 1);
                    strcpy(key, token);
                    token = strtok(NULL, " ");

                    if (token != NULL)
                    {
                        char *data = malloc(strlen(token) + 1);
                        strcpy(data, token);
                        struct data_t *dataT = data_create2(strlen(data) + 1, data);
                        struct entry_t *entry = entry_create(key, dataT);
                        if (rtable_put(tableRemote, entry) != 0)
                            printf("ERRO AO DAR PUT\n");
                        entry_destroy(entry);
                    }
                    else
                    {
                        free(key);
                        printf("FORMAT ERROR USE \"put <KEY> <DATA>\" instead\n");
                    }
                }
                else
                    printf("FORMAT ERROR USE \"put <KEY> <DATA>\" instead\n");
            }
            else if (token != NULL && strcmp(token, "get") == 0)
            {
                token = strtok(NULL, " ");
                if (token != NULL)
                {
                    char *key = malloc(strlen(token) + 1);
                    strcpy(key, token);
                    struct data_t *data = rtable_get(tableRemote, key);
                    if (data == NULL)
                        printf("ERROR GETTING\n");
                    else if (data->datasize != 0)
                    {
                        printf("DATA:%s SIZE:%d\n", (char *)data->data, data->datasize);
                        data_destroy(data);
                    }
                    else
                    {
                        printf("DATA NOT FOUND\n");
                        data_destroy(data);
                    }
                    free(key);
                }
                else
                    printf("FORMAT ERROR USE \"get <KEY>\" instead");
            }
            else if (token != NULL && strcmp(token, "del") == 0)
            {
                token = strtok(NULL, " ");
                if (token != NULL)
                {
                    char *key = malloc(strlen(token) + 1);
                    strcpy(key, token);
                    struct data_t *data;
                    if (rtable_del(tableRemote, key) == -1)
                        printf("ERROR OR KEY NOT FOUND\n");

                    free(key);
                }
                else
                    printf("FORMAT ERROR USE \"DEL <KEY>\" instead");
            }
            else if (token != NULL && strcmp(token, "size") == 0)
            {
                int size = rtable_size(tableRemote);
                if (size == -1)
                    printf("ERROR GETTING SIZE\n");
                else
                    printf("TABLE SIZE = %d\n", size);
            }
            else if (token != NULL && strcmp(token, "getkeys") == 0)
            {
                char **keys = rtable_get_keys(tableRemote);
                if (keys == NULL)
                    printf("ERROR GETTING KEYS\n");
                else
                {
                    int i = 0;
                    while (keys[i] != NULL)
                    {
                        printf("%s", keys[i]);

                        if (keys[i + 1] != NULL)
                            printf(",");
                        i++;
                    }
                    rtable_free_keys(keys);
                }
                printf("\n");
            }
            else if (token != NULL && strcmp(token, "table_print") == 0)
            {
                rtable_print(tableRemote);
                printf("\n");
            }
            else if (token != NULL && strcmp(token, "stats") == 0)
            {
                struct statistics *stats = rtable_stats(tableRemote);
                if (stats != NULL)
                {
                    printf("Average Time of request:%f\n", stats->currentAverage);
                    printf("Total Number of Requests:%ld\n", stats->n_requests);
                    printf("Number of opDel:%ld\n", stats->opDel);
                    printf("Number of opGet:%ld\n", stats->opGet);
                    printf("Number of opGetKeys:%ld\n", stats->opGetKeys);
                    printf("Number of opPut:%ld\n", stats->opPut);
                    printf("Number of opSize:%ld\n", stats->opSize);
                    printf("Number of opStats:%ld\n", stats->opStats);
                    printf("Number of opTablePrint:%ld\n", stats->opTablePrint);
                }
                free(stats);
            }
            else if (token != NULL && strcmp(token, "quit") == 0)
            {
                printf("CLOSE STATUS = %d\n", rtable_disconnect(tableRemote));
                free(lineCpy);
                break;
            }
            else
            {
                printf("Unkown Command!\n");
            }

            free(lineCpy);
            lineCpy = NULL;
        }

        printf("\n");
    }
    free(address_prot);
}