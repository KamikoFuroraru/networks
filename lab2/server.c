#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <regex.h> 

#include "server.h"
#include "navigation.h"

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

int clientCount = 0;    // number of clients online
int clientNumber = 0;   // last client id

char *paths[] = {"/animal", "/animal/cat", "/animal/dog", "/animal/cat/big", "/animal/cat/small", "/animal/dog/big", "/animal/dog/small", "/animal/cat/big/leopard", "/animal/cat/big/panther", "/animal/cat/small/home"};
int size = sizeof(paths) / sizeof(paths[0]); // SIZE OF MASSIVE OF PATHS

// gcc server.c -o server -pthread
// ./server 5000 127.0.0.1
int main(int argc, char** argv) {
    
    // check the number of entered arguments
    if (argc != 3) {
        fprintf(stderr, "Invalid cmd format. \nFormat: ./server [PORT] [IP]\n");
        exit(1);
    }
    
    int serverSocket = -1;
    int port = (*(int*) argv[1]);
    char* ip = argv[2];
    
    createServer(&serverSocket, port, ip);
    
    // create a thread where clients will be accepted
    pthread_t listeningThread;
    //listening thread function - clientConnections
    int status = pthread_create(&listeningThread, NULL, clientConnections, &serverSocket);
    checkThread(status, (char*) "\nCan't create thread because of error: %s\n");
    
    char cmd[SIZE];
    char *found;
    
    while(1) {
        fgets(cmd, SIZE, stdin);
        cmd[strlen(cmd) - 1] = '\0';
        
        // breaks the string into tokens by the specified delimiter and returns a pointer to the first token found
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
            kickAll();
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
                
                // if something else comes after the "--kikck [NUM]", the format is incorrect
                char *foundCheck = found;
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
        
    shutdown(serverSocket, SHUT_RDWR);
    printf("\nServer: socket was shut down.\n");

    close(serverSocket);
    printf("\nServer: socket was closed.\n");
    
    //waiting for the end of the listening thread
    pthread_join(listeningThread, NULL);
    
    return 0;
    
}

void* clientConnections(void* args) {
    int serverSocket = *((int*) args);
    int clientSocket;
    while(1) {
        clientSocket = accept(serverSocket, NULL, NULL);
        
        //if the server has completed its work
        if (clientSocket <= 0) {
            kickAll();
            break;
        }
        
        pthread_mutex_lock(&mutex);
        
        //create a client with its number, socket and thread
        int clientId = clientNumber;
        clients = (struct Client*) realloc(clients, sizeof(struct Client) * (clientNumber + 1));
        clients[clientId].id = clientId;
        clients[clientId].socket = clientSocket;
        
        //client thread function - clientThread
        int status = pthread_create(&(clients[clientId].clientThread), NULL, clientThread, (void*) &clientId);
        checkThread(status, (char*) "\nCan't create thread because of error: %s\n");
        
        clientCount++;
        clientNumber++;
        
        pthread_mutex_unlock(&mutex);
        
    }
    
}

void* clientThread(void* args) {    
    int clientId = *((int*)args);
    printf("\nServer: Client %d here.\n", clientId);
    
    pthread_mutex_lock(&mutex);
    int clientSocket = clients[clientId].socket;
    pthread_mutex_unlock(&mutex);

    char msg[SIZE] = {0};
    char input[PATH_SIZE] = "/animal";
    char *result;
    char *found;
    
    while(1) {

        int recieve = recvAll(clientSocket, msg);
        if (recieve <= 0 || clientCount > BACKLOG) {
            kickClient(clientId);
            break;
        }

        found = strtok(msg, " ");

        if (strcmp("ls", found) == 0) {
            result = concat("Current path: ", input);
            result = concat(result, "\n");
            result = concat(result, getChild(input, paths, size));

            printf("\nClient %d send: %s\n\n", clientId, msg);

            strcpy(msg, result);
            send(clientSocket, msg, sizeof(msg), 0);
        }

       else if (strcmp("cd", found) == 0) {
            found = strtok(NULL, " ");

            if (found == NULL) {
                strcpy(msg, "takoi cmd net");
                send(clientSocket, msg, sizeof(msg), 0);
            }

            else if (isRelative(found, paths, size) == 1) {
                strcpy(input, found);
                result = concat("Current path: ", input);
                result = concat(result, "\n");

                printf("\nClient %d send: %s\n\n", clientId, msg);

                strcpy(msg, result);
                send(clientSocket, msg, sizeof(msg), 0);
            }

            else if (strcmp("..", found) == 0) {
                strcpy(input, getParent(input));
                result = concat("Current path: ", input);
                result = concat(result, "\n");

                printf("\nClient %d send: %s\n\n", clientId, msg);

                strcpy(msg, result);
                send(clientSocket, msg, sizeof(msg), 0);
            }

            else {
                strcpy(msg, "net takogo path");
                send(clientSocket, msg, sizeof(msg), 0);
            }
       }

        else {
            strcpy(msg, "takoi cmd net");
            send(clientSocket, msg, sizeof(msg), 0);
        }
    }
}

int recvAll(int socket, char *msg) {
    int total_size = 0;
    int size_recv = 0;
    int full_size = SIZE;
    while(full_size > 0) {
        size_recv = recv(socket, msg + size_recv, full_size, 0);
        if (size_recv <= 0) {
            return -1;
        }
        total_size += size_recv;
        full_size -= total_size;
    }
    return total_size;
}

void kickAll() {
    pthread_mutex_lock(&mutex);
    int count = clientNumber;
    pthread_mutex_unlock(&mutex);
    for (int i = 0; i < count; i++) {
        kickClient(i);
    }
    printf("\nAll clients kicked.\n\n");
}

void kickClient(int id) {
    if (clients[id].socket != -1) {
        pthread_mutex_lock(&mutex);
        
        shutdown(clients[id].socket, SHUT_RDWR);
        close(clients[id].socket);
        clients[id].socket = -1;
        
        pthread_mutex_unlock(&mutex);
        
        //waiting for the end of the client thread
        pthread_join(clients[id].clientThread, NULL);
        
        clientCount--;
        printf("\nClient %d was kicked.\n\n", id);
    }
}

void showClients() {
    pthread_mutex_lock(&mutex);
    int count = clientNumber;
    pthread_mutex_unlock(&mutex);
    int number = 1;
    for (int i = 0; i < count; i++) {
        if (clients[i].socket != -1) {
            printf("\n%d) Client id = %d, socket = %d\n", number, clients[i].id, clients[i].socket);
            number++;
        }
    }
}

void findAndKick(char* number) {
    pthread_mutex_lock(&mutex);
    int count = clientNumber;
    pthread_mutex_unlock(&mutex);
    int clientId = atoi(number);
    for (int i = 0; i < count; i++) {
        bool realClient = (clients[i].id == clientId); //there is a client with this number
        bool realSocket = (clients[i].socket != -1); //client socket is alive
        if (realClient && realSocket) {
            kickClient(clientId);
            break;
        }
        else if (i+1 == count) {
            printf("\nClient %d not found.\n\n", clientId);
        }
    }
}

void* checkSocket(int* exp, char* msg) {
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

void createServer(int* serverSocket, int port, char* ip) {
    *serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    checkSocket(serverSocket, (char*) "\nCan't create socket because of error: %s\n");
    
    printf("\nServer: socket was created.\n");
    
    struct sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(port);
    serverAddress.sin_addr.s_addr = inet_addr(ip);
    
    //TIME_WAIT binding
    int opt = 1;
    if (setsockopt(*((int*)serverSocket), SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
         printf("\nCan't rebind socket because of error: %s\n", strerror(errno));
         exit(1);
    }
    
    int binding = bind(*((int*)serverSocket), (struct sockaddr*) &serverAddress, sizeof(serverAddress));
    checkSocket(&binding, (char*) "\nCan't bind socket because of error: %s\n");

    printf("\nServer: socket was binded.\n");

    if (listen(*((int*)serverSocket), BACKLOG)) {
        printf("\nCan't  do listening because of: %s\n", strerror(errno));
        exit(1);
    }
    
    printf("\nServer is listening...\n");
}