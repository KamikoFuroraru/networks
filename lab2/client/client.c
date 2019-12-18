#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <libgen.h>

#include "interaction.h"

ssize_t total=0;

void createClient(int* clientSocket, int port, char* ip);

struct Article {
    char title[SIZE];
    char author[SIZE];
    char text[SIZE];
    char infoPath[SIZE];
} *articles;

// gcc client.c -o client -pthread
// ./client 5000 127.0.0.1
int main(int argc, char** argv) {

    // check the number of entered arguments
    if (argc != 3) {
        fprintf(stderr, "Invalid cmd format. \nFormat: ./client [PORT] [IP]\n");
        exit(1);
    }
    
    int clientSocket = -1;
    int port = (*(int*) argv[1]);
    char* ip = argv[2];
    
    createClient(&clientSocket, port, ip);

    char msg[SIZE] = {0};
    
    while(1) {

        fgets(msg, sizeof(msg), stdin);
        msg[strlen(msg) - 1] = '\0';
        
        if(strcmp("exit", msg) == 0) {
            printf("\nClient: Clicked exit\n");
            break;
        }

        else if (strcmp("upload", msg) == 0) {
            send(clientSocket, msg, sizeof(msg), 0);
            readN(clientSocket, msg, sizeof(msg));

            struct Article article;

            printf("\n%s ", "Title:");
            fgets(article.title, SIZE, stdin);
            article.title[strlen(article.title) - 1] = '\0';

            printf("%s ", "Author:");
            fgets(article.author, SIZE, stdin);
            article.author[strlen(article.author) - 1] = '\0';

            printf("%s ", "Text:");
            fgets(article.text, SIZE, stdin);
            article.text[strlen(article.text) - 1] = '\0';

            article.infoPath[0] = '\0';

            send(clientSocket, &article, sizeof(struct Article), 0);

            int recieve = readN(clientSocket, msg, sizeof(msg));
            if (recieve <= 0) {
                printf("\nYou were kicked from the server\n");
                break;
            }  else {
                printf("\nUpload Succesfull\n\n");
            }

        }

        else if (strcmp("get", msg) == 0) {
            send(clientSocket, msg, sizeof(msg), 0);
            readN(clientSocket, msg, sizeof(msg));

            printf("\n%s ", "Title:");
            fgets(msg, SIZE, stdin);
            msg[strlen(msg) - 1] = '\0';

            send(clientSocket, msg, sizeof(msg), 0);

            int recieve = readN(clientSocket, msg, sizeof(msg));
            if (recieve <= 0) {
                printf("\nYou were kicked from the server\n");
                break;
            }  else {
                printf("Text of the article: %s\n", msg);
            }


        }

        else if (strcmp("get-all", msg) == 0) {
            send(clientSocket, msg, sizeof(msg), 0); // send cmd get-all
            readN(clientSocket, msg, sizeof(msg));

            printf("\n%s ", "Author:");
            fgets(msg, SIZE, stdin);
            msg[strlen(msg) - 1] = '\0';

            send(clientSocket, msg, sizeof(msg), 0); // send author name

            readN(clientSocket, msg, sizeof(msg)); // read found count
            int exist = atoi(msg);
            send(clientSocket, msg, sizeof(msg), 0);

            if (exist > 0) {

                for (int i = 0; i < exist; i++) {

                    readN(clientSocket, msg, sizeof(msg));
                    printf("\nTitle: %s\n", msg);
                    send(clientSocket, msg, sizeof(msg), 0);

                    readN(clientSocket, msg, sizeof(msg));
                    printf("Text: %s\n", msg);
                    send(clientSocket, msg, sizeof(msg), 0);

                }
            }

            else {
                printf("\nStatei takogo avtora v dannoi directory nothing\n");
            }


            readN(clientSocket, msg, sizeof(msg));


            printf("\n%s\n\n", "Get-all Succesfull");

        }

        else {
            send(clientSocket, msg, sizeof(msg), 0);
            int recieve = readN(clientSocket, msg, sizeof(msg));       
            if (recieve <= 0) {
                printf("\nYou were kicked from the server\n");
                break;
            }  else {
                printf("\n%s\n\n", msg); 
            }
        }

    }

    shutdown(clientSocket, SHUT_RDWR);
    printf("\nClient: socket was shut down.\n");

    close(clientSocket);
    printf("\nClient: socket was closed.\n");

    return 0;

}

void createClient(int* clientSocket, int port, char* ip) {
    *clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    
    if (*((int*)clientSocket) < 0) {
        printf("\nCan't create socket because of error: %s\n", strerror(errno));
        exit(1);
    }
    printf("\nClient socket was created.\n\n");
    
    struct sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(port);
    serverAddress.sin_addr.s_addr = inet_addr(ip);
    
    int connectionStatus = connect(*((int*)clientSocket), (struct sockaddr *) &serverAddress, sizeof(serverAddress));
    
    if (connectionStatus == -1) {
        printf("\nCan't making connection to the socket because of error: %s\n", strerror(errno));
        exit(1);
    }
    
}