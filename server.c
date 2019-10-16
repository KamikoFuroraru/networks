#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>

#define PORT 5000
#define SIZE 256
#define BACKLOG 2

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

struct Client {
    pthread_t clientThread;
    int socket;
    int id;
} *clients;

typedef enum {true, false} bool;

int clientCount = 0;
int clientNumber = 0;

void* clientThread(void* args);
int recvAll(int socket, char* str);
void* clientConnections(void* args);
void* checkSocket(int exp, char* msg);
void* checkThread(int thread, char* msg);
void* clientThread(void* args);
void kickClient(int id);
void kickAllClients();
void showClients();
void findAndKick();
void createServer(int serverSocket);

int main(int argc, char** argv) {
    
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    createServer(serverSocket);
    
    pthread_t listeningThread;
    int status = pthread_create(&listeningThread, NULL, clientConnections, &serverSocket);
    checkThread(status, (char*) "\nCan't create thread because of error: %s\n");
    
    char cmd[SIZE];
    char *found;
    
    while(1) {
        fgets(cmd, SIZE, stdin);
        cmd[strlen(cmd) - 1] = '\0';
        
        found = strtok(cmd, " ");
    
        if (found == NULL) {
            printf("Enter the cmd\n\n");
            continue;
        }
        
        else if(strcmp("--exit", found) == 0) {            
            break;
        }
        
        else if (strcmp("--kick-all", found) == 0) {
            if (clientCount == 0) {
                printf("\nThere is no one clients on the server.\n\n");
                continue;
            }
            kickAllClients();
        }
        
        else if (strcmp("--show-clients", found) == 0) {
            printf("Count of clients: %d", clientCount);
            if (clientCount == 0) {
                printf("\nThere is no one clients on the server.\n\n");
                continue;
            }
            showClients();
        }
        
        else if (strcmp("--kick", found) == 0) {  
                if (clientCount == 0) {
                    printf("\nThere is no one clients on the server.\n\n");
                    continue;
                }
                
                found = strtok(NULL, " ");
                
                char *foundCheck = found; //check next
                foundCheck = strtok(NULL, " ");
                
                if (foundCheck != NULL) {
                    printf("\nWrong cmd format.\n\n");
                    continue;
                }
                
                findAndKick(found);
                
            }
        
        else { 
            printf("There is no such command\n");
            continue;
        }
        
    }
    
    //pthread_join(listeningThread, NULL);
    
    shutdown(serverSocket, BACKLOG);
    printf("\nServer: socket was shut down.\n");

    close(serverSocket);
    printf("\nServer: socket was closed.\n");
    
    return 0;
    
}

void* clientConnections(void* args) {
    int serverSocket = *((int*) args);
    int clientSocket;
    while(1) {
        clientSocket = accept(serverSocket, NULL, NULL);
        
        //pthread_mutex_lock(&mutex);
        
        int clientId = clientNumber;
        clients = (struct Client*) realloc(clients, sizeof(struct Client) * (clientNumber + 1));
        clients[clientId].id = clientId;
        clients[clientId].socket = clientSocket;

        int status = pthread_create(&(clients[clientId].clientThread), NULL, clientThread, (void*) &clientId);
        checkThread(status, (char*) "\nCan't create thread because of error: %s\n");
        
        clientCount++;
        clientNumber++;
        
        //pthread_mutex_unlock(&mutex);
        
    }
    
}

void* clientThread(void* args) {    
    int clientId = *((int*)args);
    printf("\nServer: Client %d here.\n", clientId);
    
    //pthread_mutex_lock(&mutex);
    int clientSocket = clients[clientId].socket;
    //pthread_mutex_unlock(&mutex);

    char msg[SIZE] = {0};
    
    while(1) {
        if (recvAll(clientSocket, msg) <= 0 || clientCount > BACKLOG) {
            kickClient(clientId);
            break;
        }
        printf("\nClient %d send: %s\n\n", clientId, msg);
        send(clientSocket, msg, sizeof(msg), 0);
        
    }
}

int recvAll(int socket, char* str) {
    int result = 0;
    int recved = 0;
    int size = SIZE;
    while(size > 0) {
        recved = recv(socket, str + recved, size, 0);
        if (recved <= 0) {
            return -1;
        }
        result += recved;
        size -= result;
    }
    return result;
}

void kickAllClients() {
    //pthread_mutex_lock(&mutex);
    int count = clientNumber;
    //pthread_mutex_unlock(&mutex);
    for (int i = 0; i < count; i++) {
        kickClient(i);
    }
    printf("\nAll clients kicked.\n\n");
}

void kickClient(int id) {
    if (clients[id].socket != -1) {
        shutdown(clients[id].socket, SHUT_RDWR);
        close(clients[id].socket);
        
        clients[id].socket = -1;
        pthread_join(clients[id].clientThread, NULL);
        
        clientCount--;
        printf("\nClient %d was kicked.\n\n", id);
    }
}

void showClients() {
    //pthread_mutex_lock(&mutex);
    int count = clientNumber;
    //pthread_mutex_unlock(&mutex);
    int number = 1;
    for (int i = 0; i < count; i++) {
        if (clients[i].socket != -1) {
            printf("\n%d) Client id = %d, socket = %d\n", number, clients[i].id, clients[i].socket);
            number++;
        }
    }
}

void findAndKick(char* number) {
    //pthread_mutex_lock(&mutex);
    int count = clientNumber;
    //pthread_mutex_unlock(&mutex);
    int clientId = atoi(number);
    for (int i = 0; i < count; i++) {
        bool realClient = (clients[i].id == clientId);
        bool realSocket = (clients[i].socket != -1);
        if (realClient && realSocket) {
            kickClient(clientId);
            break;
        }
        else if (i+1 == count) {
            printf("\nClient %d not found.\n\n", clientId);
        }
    }
}

void* checkSocket(int exp, char* msg) {
    if (exp < 0) {
        printf(msg, strerror(errno));
        exit(1);
    }
}

void* checkThread(int thread, char* msg) {
    if (thread != 0) {
        printf(msg, strerror(errno));
        exit(1);
    }
}

void createServer(int serverSocket) {
    checkSocket(serverSocket, (char*) "\nCan't create socket because of error: %s\n");
    
    printf("\nServer: socket was created.\n");
    
    struct sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(PORT);
    serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
    
    //TIME_WAIT binding
    int opt = 1;
    if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
         printf("\nCan't rebind socket because of error: %s\n", strerror(errno));
         exit(1);
    }
    
    int binding = bind(serverSocket, (struct sockaddr*) &serverAddress, sizeof(serverAddress));
    checkSocket(binding, (char*) "\nCan't bind socket because of error: %s\n");

    printf("\nServer: socket was binded.\n");

    if (listen(serverSocket, BACKLOG)) {
        printf("\nCan't  do listening because of: %s\n", strerror(errno));
        exit(1);
    }
    
    printf("\nServer is listening...\n");
}
