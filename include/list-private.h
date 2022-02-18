#ifndef _LIST_PRIVATE_H
#define _LIST_PRIVATE_H
/*Realizado por:
-João Ferraz, nº49420.
-Miguel Carvalho, nº54399;
-Bruno Cotrim, nº54406;*/
#include "list.h"
struct node_t
{
    struct entry_t *data;
    struct node_t *next;
};

struct list_t
{
    int size;
    struct node_t *head;
};

void list_print(struct list_t *list);

#endif
