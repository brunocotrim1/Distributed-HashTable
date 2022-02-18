/*Realizado por:
-João Ferraz, nº49420.
-Miguel Carvalho, nº54399;
-Bruno Cotrim, nº54406;
*/
#include "network_server.h"
#include <stdio.h>
#include <string.h>
void test_parameters(char *argument)
{
    if (strlen(argument) == 0)
    {
        printf("'%s' O argumento de entrada deve conter apenas números\n", argument);
        exit(-1);
    }
    char *endptr;
    int number;
    number = strtol(argument, &endptr, 10);

    if (*endptr != '\0' || endptr == argument)
    {
        printf("'%s' O argumento de entrada deve conter apenas números\n", argument);
        exit(-1);
    }
}

int main(int argc, char **argv)
{
    if (argc != 4)
    {
        printf("Número de argumentos errado, insira: ./table_client <port> <n_lists> <Zookeeper_Address_port>");
        exit(-1);
    }

    test_parameters(argv[1]);
    test_parameters(argv[2]);
    table_skel_init(atoi(argv[2]));
    int socket_de_escuta = network_server_init((short)atoi(argv[1]), argv[3]);
    int result = network_main_loop(socket_de_escuta);
    table_skel_destroy();
    return 0;
}