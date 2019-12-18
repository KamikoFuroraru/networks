#include "createServer.h"
#include "interaction.h"

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

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

