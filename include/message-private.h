#ifndef _MESSAGE_PRIVATE_H
#define _MESSAGE_PRIVATE_H
#include <sys/socket.h>
#include <stdint.h>
#include <sys/types.h>
int send_all(int socket, uint8_t *buffer, int length);

int receive_all(int socket, uint8_t *buffer,unsigned len);
#endif