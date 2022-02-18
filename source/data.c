/*Realizado por:
-João Ferraz, nº49420.
-Miguel Carvalho, nº54399;
-Bruno Cotrim, nº54406;*/

#include <data.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
/* Função que cria um novo elemento de dados data_t e reserva a memória
 * necessária, especificada pelo parâmetro size 
 */
struct data_t *data_create(int size)
{
	if (size <= 0)
		return NULL;
	struct data_t *ptr = (struct data_t *)malloc(sizeof(struct data_t));
	ptr->datasize = size;
	ptr->data = (void *)malloc(size);
	return ptr;
}

/* Função idêntica à anterior, mas que inicializa os dados de acordo com
 * o parâmetro data.
 */
struct data_t *data_create2(int size, void *data)
{
	if (data == NULL || size < 0)//size <= 0 
		return NULL;
	struct data_t *ptr = (struct data_t *)malloc(sizeof(struct data_t));
	ptr->datasize = size;

	ptr->data = data;

	return ptr;
}
/* Função que elimina um bloco de dados, apontado pelo parâmetro data,
 * libertando toda a memória por ele ocupada.
 */
void data_destroy(struct data_t *data)
{
	if (data == NULL)
		return;

	free(data->data);
	data->data = NULL;
	free(data);
	data = NULL;
}

/* Função que duplica uma estrutura data_t, reservando a memória
 * necessária para a nova estrutura.
 */
struct data_t *data_dup(struct data_t *data)
{

	if (data == NULL || data->data == NULL || data->datasize <= 0)
	{
		return NULL;
	}
	else
	{
		void *copy = malloc(data->datasize);
		memcpy(copy, data->data, data->datasize);
		return data_create2(data->datasize, copy);
	}
}

/* Função que substitui o conteúdo de um elemento de dados data_t.
*  Deve assegurar que destroi o conteúdo antigo do mesmo.
*/
void data_replace(struct data_t *data, int new_size, void *new_data)
{
	data->data = new_data;
	data->datasize = new_size;
}
