#ifndef SERVER_H_
#define SERVER_H_

void* clientThread(void* args);
int recvAll(int socket, char* str);
void* clientConnections(void* args);
void* checkSocket(int* exp, char* msg);
void* checkThread(int thread, char* msg);
void kickClient(int id);
void kickAll();
void showClients();
void findAndKick();
void createServer(int* serverSocket, int port, char* ip);

struct Client {
    pthread_t clientThread;
    int socket;
    int id;
} *clients;

typedef enum {true, false} bool;

#define SIZE 256
#define BACKLOG 2

#endif