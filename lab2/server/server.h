#ifndef SERVER_H_
#define SERVER_H_

void* clientThread(void* args);
int recvAll(int socket, char* str);
void* clientConnections(void* args);
void kickClient(int id);
void kickAll();
void showClients();
void findAndKick();

struct Client {
    pthread_t clientThread;
    int socket;
    int id;
} *clients;

typedef enum {true, false} bool;

#endif