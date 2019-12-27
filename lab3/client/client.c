// Client side implementation of UDP client-server model
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>

#define MAXLINE 1024
#define OK 200
#define ERROR 400
#define TIMEOUT 5
#define KICK 300

int numberMsg = 0;  // started value for msg (normal = 0, timeout = 1)
int num = 1; // перемешивание = 1 и --, отправка старых -1 и ++, дублирование не меняется

int kicking = 0;

struct Package {
    int number;
    int errorCode;
    int count;
    char cmd[MAXLINE];
    char data[MAXLINE];
};

struct args {
    int socket;
    struct sockaddr_in servaddr;
};

void *listenServer(void *input);

// Driver code
int main(int argc, char **argv) {

    if (argc != 3) {
        fprintf(stderr, "Invalid cmd format. \nFormat: ./client [PORT] [IP]\n");
        exit(1);
    }

    int clientSocket = -1;
    int port = (*(int *) argv[1]);
    char *ip = argv[2];
    char *found;

    // Creating socket file descriptor
    if ((clientSocket = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Filling server information
    struct sockaddr_in servaddr;
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(port);
    servaddr.sin_addr.s_addr = inet_addr(ip);

    struct args *toThread = (struct args *)malloc(sizeof(struct args));
    toThread->socket = clientSocket;
    toThread->servaddr = servaddr;

    pthread_t listeningThread;
    int status = pthread_create(&listeningThread, NULL, listenServer, (void *) toThread);

    char msg[MAXLINE] = {"\0"};
    int len;
    struct Package sendPackage;

    while(1) {

        if (kicking == 1) break;

        fgets(msg, sizeof(msg), stdin);
        msg[strlen(msg) - 1] = '\0';
        
        if(strcmp("exit", msg) == 0) {
            printf("\nClient: Clicked exit\n");
            break;
        }

        else if (strcmp("upload", msg) == 0) {
            sendPackage.count = TIMEOUT;
            sendPackage.errorCode = OK;
            sendPackage.number = numberMsg;
            strcpy(sendPackage.cmd, msg);

            printf("\n%s ", "Title:");
            fgets(sendPackage.data, MAXLINE, stdin);

            printf("%s ", "Author:");
            fgets(msg, MAXLINE, stdin);
            strcat(sendPackage.data, msg);

            printf("%s ", "Text:");
            fgets(msg, MAXLINE, stdin);
            strcat(sendPackage.data, msg);

            if (kicking != 1) {
                sendto(clientSocket, &sendPackage, sizeof(struct Package), 0, (const struct sockaddr *) &servaddr, sizeof(servaddr));
                printf("\nMessage #%d sent.\n", numberMsg);
            }

        }

        else if (strcmp("get", msg) == 0) {
            sendPackage.count = TIMEOUT;
            sendPackage.errorCode = OK;
            sendPackage.number = numberMsg;
            strcpy(sendPackage.cmd, msg);

            printf("\n%s ", "Title:");
            fgets(msg, MAXLINE, stdin);
            msg[strlen(msg) - 1] = '\0';

            strcpy(sendPackage.data, msg);

            if (kicking != 1) {
                sendto(clientSocket, &sendPackage, sizeof(struct Package), 0, (const struct sockaddr *) &servaddr, sizeof(servaddr));
                printf("\nMessage #%d sent.\n", numberMsg);
            }
        }

        else if (strcmp("get-all", msg) == 0) {
            sendPackage.count = TIMEOUT;
            sendPackage.errorCode = OK;
            sendPackage.number = numberMsg;
            strcpy(sendPackage.cmd, msg);

            printf("\n%s ", "Author:");
            fgets(msg, MAXLINE, stdin);
            msg[strlen(msg) - 1] = '\0';

            strcpy(sendPackage.data, msg);

            if (kicking != 1) {
                sendto(clientSocket, &sendPackage, sizeof(struct Package), 0, (const struct sockaddr *) &servaddr, sizeof(servaddr));
                printf("\nMessage #%d sent.\n", numberMsg);
            }
        }

        else {
            sendPackage.count = TIMEOUT;
            sendPackage.errorCode = OK;
            sendPackage.number = numberMsg;//
            strcpy(sendPackage.cmd, msg);

            if (kicking != 1) {
                sendto(clientSocket, &sendPackage, sizeof(struct Package), 0, (const struct sockaddr *) &servaddr, sizeof(servaddr));
                printf("\nMessage #%d sent.\n", numberMsg);//
            }
            //num--;

        }

        numberMsg++;

    }

    pthread_join(listeningThread, NULL);

    shutdown(clientSocket, SHUT_RDWR);
    printf("\nClient: socket was shut down.\n");

    close(clientSocket);
    printf("\nClient: socket was closed.\n");

    return 0;
}

void *listenServer(void *input) {
    int socket = ((struct args*)input)->socket;
    struct sockaddr_in servaddr = ((struct args*)input)->servaddr;

    struct Package recvPackage;
    char msg[MAXLINE] = {"\0"};
    int len;
    char *found;

    while(1) {

        recvfrom(socket, &recvPackage, sizeof(struct Package), 0, (struct sockaddr *) &servaddr, &len);

        if (recvPackage.errorCode == ERROR) {
            recvPackage.count--;
            if (recvPackage.count > 0) {
                recvPackage.errorCode = OK;
                sendto(socket, &recvPackage, sizeof(struct Package), 0, (const struct sockaddr *) &servaddr, sizeof(servaddr));
                printf("\nMessage #%d resent.\n", recvPackage.number);
            }
            else printf("\nMessage [%s] with #%d call the TIMEOUT. Please, try again.\n", recvPackage.cmd, recvPackage.number);
        }

        else if (recvPackage.errorCode == KICK) {
            printf("%s\n", "kicking");
            kicking = 1;
            break;
        }

        else {
            if (strcmp("get", recvPackage.cmd) == 0) {
                found = strtok(recvPackage.data, "\n");
                printf("Text of article : %s\n", found);
                found = strtok(NULL, "\n");
                printf("Author : %s\n\n", found);
            }
            else if (strcmp("get-all", recvPackage.cmd) == 0) {
                found = strtok(recvPackage.data, "\n");
                while(found != NULL) {
                    printf("Text of article : %s\n", found);
                    found = strtok(NULL, "\n");
                    printf("Title : %s\n\n", found);
                    found = strtok(NULL, "\n");
                }
            }
            else printf("\nServer: \n%s\n", recvPackage.data);
        }
    }

}