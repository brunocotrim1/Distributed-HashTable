#ifndef _TABLE_PRIVATE_H
#define _TABLE_PRIVATE_H
/*Realizado por:
-João Ferraz, nº49420.
-Miguel Carvalho, nº54399;
-Bruno Cotrim, nº54406;*/
#include "list.h"
#include "list-private.h"
struct table_t {
    struct list_t** buckets;
    int size;
    int n_elements;
};

int hashTabela(char *key, int m);

#endif
