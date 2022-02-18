/*Realizado por:
-João Ferraz, nº49420.
-Miguel Carvalho, nº54399;
-Bruno Cotrim, nº54406;*/
#include <entry.h>

#include <string.h>
#include <stdlib.h>
/* Função que cria uma entry, reservando a memória necessária e
 * inicializando-a com a string e o bloco de dados passados.
 */
struct entry_t *entry_create(char *key, struct data_t *data)
{
    struct entry_t *entry = (struct entry_t *)malloc(sizeof(struct entry_t));
    entry_initialize(entry);
    entry->key = key;
    entry->value = data;

    return entry;
}

/* Função que inicializa os elementos de uma entrada na tabela com o
 * valor NULL.
 */
void entry_initialize(struct entry_t *entry)
{
    entry->key = NULL;
    entry->value = NULL;
}

/* Função que elimina uma entry, libertando a memória por ela ocupada
 */
void entry_destroy(struct entry_t *entry)
{

    if (entry != NULL)
    {
        data_destroy(entry->value);
        free(entry->key);
        entry->key = NULL;
        free(entry);
        entry = NULL;
    }
}

/* Função que duplica uma entry, reservando a memória necessária para a
 * nova estrutura.
 */
struct entry_t *entry_dup(struct entry_t *entry)
{
    char *keyCpy = malloc(strlen(entry->key));
    memcpy(keyCpy, entry->key, strlen(entry->key) + 1);
    struct data_t *valueCpy = data_dup(entry->value);
    return entry_create(keyCpy, valueCpy);
}

/* Função que substitui o conteúdo de uma entrada entry_t.
*  Deve assegurar que destroi o conteúdo antigo da mesma.
*/
void entry_replace(struct entry_t *entry, char *new_key, struct data_t *new_value)
{
    free(entry->key);
    entry->key = NULL;
    free(entry->value);
    entry->value = NULL;
    entry->value = new_value;
    entry->key = new_key;
}

/* Função que compara duas entradas e retorna a ordem das mesmas.
*  Ordem das entradas é definida pela ordem das suas chaves.
*  A função devolve 0 se forem iguais, -1 se entry1<entry2, e 1 caso contrário.
*/
int entry_compare(struct entry_t *entry1, struct entry_t *entry2)
{
    int val = strcmp(entry1->key, entry2->key);

    if (entry1->key == entry2->key || val == 0)
    {
        return 0;
    }
    else
        return val;
}
