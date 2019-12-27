// Server side implementation of UDP client-server model 
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <regex.h> 

#include "interaction.h"

#define MAXLINE 1024
#define OK 200
#define ERROR 400
#define KICK 300

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

char *paths[] = {"/animal", "/animal/cat", "/animal/dog", "/animal/cat/big", "/animal/cat/small", "/animal/dog/big",
                 "/animal/dog/small", "/animal/cat/big/leopard", "/animal/cat/big/panther", "/animal/cat/small/home"};
int size = sizeof(paths) / sizeof(paths[0]); // SIZE OF MASSIVE OF PATHS
typedef enum {true, false} bool;

struct Package {
    int number;
    int errorCode;
    int count;
    char cmd[MAXLINE];
    char data[MAXLINE];
};

struct Client {
    int id;
    int port;
    int prevMsgNumber;
    char curPath[MAXLINE];
} *clients;

struct Article {
    char title[MAXLINE];
    char author[MAXLINE];
    char text[MAXLINE];
    char infoPath[MAXLINE];
} *articles;

struct args {
    int socket;
    struct Package package;
    struct sockaddr_in cliaddr;
};

int articleNumber = 0;
int clientNumber = 0;   // last client id
int clientCount = 0;  // client count

void *listenClientCmd(void *args);
void *doCmd(void *input);
void showClients();
void findAndKick(char* number, int socket);
void kickClient(int id, int socket);
void kickAll(int socket);

struct sockaddr_in servaddr, cliaddr;

// Driver code 
int main(int argc, char **argv) {

    if (argc != 2) {
        fprintf(stderr, "Invalid cmd format. \nFormat: ./server [PORT]\n");
        exit(1);
    }

    int serverSocket = -1;
    int port = (*(int *) argv[1]);

    // Creating socket file descriptor 
    if ((serverSocket = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Filling server information 
    servaddr.sin_family = AF_INET; // IPv4 
    servaddr.sin_port = htons(port);
    servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");

    // Bind the socket with the server address 
    if (bind(serverSocket, (const struct sockaddr *) &servaddr, sizeof(servaddr)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    pthread_t listeningThread;
    //listening thread function - clientConnections
    int status = pthread_create(&listeningThread, NULL, listenClientCmd, &serverSocket);

    char cmd[MAXLINE] = {"\0"};;
    char *found;

    while (1) {

        fgets(cmd, MAXLINE, stdin);
        cmd[strlen(cmd) - 1] = '\0';

        // breaks the string into tokens by the specified delimiter and returns a pointer to the first token found
        found = strtok(cmd, " ");

        if (found == NULL) {
            printf("Enter the cmd\n\n");
            continue;
        } else if (strcmp("exit", found) == 0) {
            pthread_cancel(listeningThread);
            break;
        }

        else if (strcmp("clients", found) == 0) {
            printf("Count of clients: %d", clientCount);
            if (clientCount == 0) {
                printf("\nThere is no one clients on the server.\n\n");
                continue;
            }
            showClients();
        }

        else if (strcmp("kick", found) == 0) {  
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

            findAndKick(found, serverSocket);
        }

        else if (strcmp("kick-all", found) == 0) {
            if (clientCount == 0) {
                printf("\nThere is no one clients on the server.\n\n");
                continue;
            }
            kickAll(serverSocket);
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

void *listenClientCmd(void *args) {

    int serverSocket = *((int *) args);
    int len = sizeof(cliaddr);  //len is value/resuslt 

    char msg[MAXLINE] = {"\0"};
    char *found;
    struct Package package;
    struct args *toThread = (struct args *)malloc(sizeof(struct args));

    while (1) {

        recvfrom(serverSocket, &package, sizeof(struct Package), 0, (struct sockaddr *) &cliaddr, &len);
        
        toThread->socket = serverSocket;
        toThread->package = package;
        toThread->cliaddr = cliaddr;

        pthread_t thread;
        pthread_create(&thread, NULL, doCmd, (void *) toThread);
    }
}


void *doCmd(void *input) {
    int socket = ((struct args*)input)->socket;
    struct Package recvPackage = ((struct args*)input)->package;
    struct sockaddr_in cliaddr = ((struct args*)input)->cliaddr;

    int exist = 0;
    int clientId = clientNumber;

    for (int i = 0; i < clientId; i++) {
        if (clients[i].port == cliaddr.sin_port) {
            clientId = i;
            exist = 1;
            break;
        }
    }

    if (exist == 0) {
        clients = (struct Client*) realloc(clients, sizeof(struct Client) * (clientId + 1));
        clients[clientId].id = clientId;
        clients[clientId].port = cliaddr.sin_port;
        clients[clientId].prevMsgNumber = -1;
        strcpy(clients[clientId].curPath, "/animal");
        clientNumber++;
        clientCount++;
    }

    char msg[MAXLINE] = {"\0"};
    char *found;
    struct Package sendPackage;
    int prevMsgNumber = recvPackage.number - 1; ////////// перемешивание 
    int nextMsgNumber = clients[clientId].prevMsgNumber + 1;

    if (clients[clientId].prevMsgNumber == prevMsgNumber) {

        strcpy(msg, recvPackage.cmd);
        printf("Client with id [%d] send [%s]\n", clientId, msg);

        found = strtok(msg, " ");

        if (found == NULL) {
            sendPackage.number = recvPackage.number;
            strcpy(sendPackage.cmd, recvPackage.cmd);
            strcpy(sendPackage.data, "Takoi cmd net\n");

        }

        else if (strcmp("ls", found) == 0) { /////////////////    ls     /////////////////
            char art[SIZE] = {"\0"};
            exist = 0;

            strcpy(sendPackage.data, "Current path: ");
            strcat(sendPackage.data, clients[clientId].curPath);
            strcat(sendPackage.data, "\n\n");

            strcat(sendPackage.data, "Next paths: \n");
            strcat(sendPackage.data, getChild(clients[clientId].curPath, paths, size));

            strcpy(art, "\n");
            strcat(art, "Articles: ");
            for (int i = 0; i < articleNumber; i++) {
                if (strcmp(articles[i].infoPath, clients[clientId].curPath) == 0) {
                    strcat(art, "\n");
                    strcat(art, articles[i].title);
                    exist++;
                }
            }
            strcat(art, "\n");
            if (exist > 0) strcat(sendPackage.data, art);

            sendPackage.number = recvPackage.number;
            strcpy(sendPackage.cmd, recvPackage.cmd);

        }

        else if (strcmp("cd", found) == 0) {  //////////////////    cd      ///////////////////
            char regexp[SIZE];
            regex_t regex;
            int compiledRegex;

            found = strtok(NULL, " ");

            if (found == NULL) {
                strcpy(sendPackage.data, "takoi cmd net");
            }

            else if (isRelative(found, paths, size) == 1) {
                strcpy(clients[clientId].curPath, found);

                strcpy(sendPackage.data, "Current path: ");
                strcat(sendPackage.data, clients[clientId].curPath);
                strcat(sendPackage.data, "\n");
            }

            else if (strcmp("..", found) == 0) {
                strcpy(clients[clientId].curPath, getParent(clients[clientId].curPath));

                strcpy(sendPackage.data, "Current path: ");
                strcat(sendPackage.data, clients[clientId].curPath);
                strcat(sendPackage.data, "\n");
            }

            else {
                strcpy(regexp, found);
                strcat(regexp, "\n");
                compiledRegex = regcomp(&regex, regexp, REG_EXTENDED);
                compiledRegex = regexec(&regex, getChild(clients[clientId].curPath, paths, size), 0, NULL, 0);

                if (compiledRegex == 0) {  // found reg exp
                    strcat(clients[clientId].curPath, "/");
                    strcat(clients[clientId].curPath, found);
                    strcpy(sendPackage.data, clients[clientId].curPath);
                    strcat(sendPackage.data, "\n");
                }

                else strcpy(sendPackage.data, "net takogo path");
            }    

        } 

        else if (strcmp("upload", found) == 0) { /////////////////      upload       /////////////////
            articles = (struct Article*) realloc(articles, sizeof(struct Article) * (articleNumber + 1));

            found = strtok(recvPackage.data, "\n");
            strcpy(articles[articleNumber].title, found);
            found = strtok(NULL, "\n");
            strcpy(articles[articleNumber].author, found);
            found = strtok(NULL, "\n");
            strcpy(articles[articleNumber].text, found);
            strcpy(articles[articleNumber].infoPath, clients[clientId].curPath);

            sleep(5);

            strcpy(sendPackage.data, "Server got article succesfull.\n");
            articleNumber++;
        }

        else if (strcmp("get", found) == 0) { ////////////////         get         /////////////////

            exist = 0;

            for (int i = 0; i < articleNumber; i++) {
                if ((strcmp(articles[i].title, recvPackage.data) == 0) &&  (strcmp(articles[i].infoPath, clients[clientId].curPath) == 0)) {
                    strcpy(sendPackage.data, articles[i].text);
                    strcat(sendPackage.data, "\n");
                    strcat(sendPackage.data, articles[i].author);
                    exist++;
                    break;
                }
            }

            if (exist == 0) strcpy(sendPackage.data, "Statii s takim title net\n");

        }

        else if (strcmp("get-all", found) == 0) { ////////////////     get-all         /////////////////

            exist = 0;

            for (int i = 0; i < articleNumber; i++) {
                if ((strcmp(articles[i].author, recvPackage.data) == 0) && (strcmp(articles[i].infoPath, clients[clientId].curPath) == 0)) {
                    exist++;
                }
            }

            strcpy(sendPackage.data, "\0");

            if (exist > 0) {
                for (int i = 0; i < articleNumber; i++) {
                    if ((strcmp(articles[i].author, recvPackage.data) == 0) &&  (strcmp(articles[i].infoPath, clients[clientId].curPath) == 0)) {
                        strcpy(msg, articles[i].title);
                        strcat(msg, "\n");
                        strcat(msg, articles[i].text);
                        strcat(msg, "\n");
                        strcat(sendPackage.data, msg);
                    }
                }
            }

            else {
                strcpy(sendPackage.data, "Statii s takim authorom net\n");
            }

        }

        else strcpy(sendPackage.data, "Takoi cmd net\n");
        clients[clientId].prevMsgNumber++;
        sendPackage.errorCode = OK;
    }

    else {
        sprintf(msg, "Waited for message with number %d but recieved %d", nextMsgNumber, recvPackage.number);
        strcpy(sendPackage.data, msg);
        sendPackage.errorCode = ERROR;
    }

    sendPackage.count = recvPackage.count;
    sendPackage.number = recvPackage.number;
    strcpy(sendPackage.cmd, recvPackage.cmd);

    sendto(socket, &sendPackage, sizeof(struct Package), 0, (const struct sockaddr *) &cliaddr, sizeof(cliaddr));
    printf("Message #%d sent to client %d.\n\n", sendPackage.number, clientId);

    pthread_exit(NULL);

}

void showClients() {
    pthread_mutex_lock(&mutex);
    int count = clientNumber;
    pthread_mutex_unlock(&mutex);
    int number = 1;
    for (int i = 0; i < count; i++) {
        if (clients[i].port != -1) {
            printf("\n%d) Client id = %d\n", number, clients[i].id);
            number++;
        }
    }
}

void findAndKick(char* number, int socket) {
    pthread_mutex_lock(&mutex);
    int count = clientNumber;
    pthread_mutex_unlock(&mutex);
    int clientId = atoi(number);
    for (int i = 0; i < count; i++) {
        bool realClient = (clients[i].id == clientId); //there is a client with this number
        bool realPort = (clients[i].port != -1); //client socket is alive
        if (realClient && realPort) {
            kickClient(clientId, socket);
            break;
        }
        else if (i+1 == count) {
            printf("\nClient %d not found.\n\n", clientId);
        }
    }
}

void kickClient(int id, int socket) {
    struct Package package;
    if (clients[id].port != -1) {
        pthread_mutex_lock(&mutex);
        printf("%d\n", clients[id].port);
        clients[id].port = -1;
        pthread_mutex_unlock(&mutex);

        package.errorCode = KICK;
        sendto(socket, &package, sizeof(struct Package), 0, (const struct sockaddr *) &cliaddr, sizeof(cliaddr));
        clientCount--;
        printf("\nClient %d was kicked.\n\n", id);
    }
}

void kickAll(int socket) { // не работает
    pthread_mutex_lock(&mutex);
    int count = clientNumber;
    pthread_mutex_unlock(&mutex);
    for (int i = 0; i < count; i++) {
        kickClient(i, socket);
    }
    printf("\nAll clients kicked.\n\n");
}
