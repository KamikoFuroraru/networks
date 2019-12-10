#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#define SIZE 256

void createClient(int* clientSocket, int port, char* ip);
int recvAll(int socket, char* msg);

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
        
        if(strcmp("--exit", msg) == 0) {
            printf("\nClient: Clicked exit\n");
            break;
        }
            
        send(clientSocket, msg, sizeof(msg), 0);

        int recieve = recvAll(clientSocket, msg);
                
        if (recieve <= 0) {
             printf("\nYou were kicked from the server\n");
             break;
 
         }  else {
                 printf("\nInfo system: %s\n\n", msg); 
         }

    }
        
    shutdown(clientSocket, SHUT_RDWR);
    printf("\nClient: socket was shut down.\n");

    close(clientSocket);
    printf("\nClient: socket was closed.\n");

    return 0;

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