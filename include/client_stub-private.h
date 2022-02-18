#ifndef _CLIENT_STUB_PRIVATE_H
#define _CLIENT_STUB_PRIVATE_H
#define MAX_MSG 2048
struct rtable_t
{
    char *adress;
    short port;
    int socketfd;
};

#endif