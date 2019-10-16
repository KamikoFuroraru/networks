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

void createClient(int clientSocket);
int recvAll(int socket, char* str);
void closeClient();
pthread_mutex_t mutex;
int smth = 0;

int main(int argc, char** argv) {
    
    int clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    createClient(clientSocket);
    
    pthread_mutex_init(&mutex, NULL);
        
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
                 printf("\nEcho SERVER: %s\n\n", msg); 
         }

    }
    
    closeClient(clientSocket);

    return 0;

    
}

void closeClient(int clientSocket) {
    
    shutdown(clientSocket, SHUT_RDWR);
    printf("\nClient: socket was shut down.\n");

    close(clientSocket);
    printf("\nClient: socket was closed.\n");

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

void createClient(int clientSocket) {
    if (clientSocket < 0) {
        printf("\nCan't create socket because of error: %s\n", strerror(errno));
        exit(1);
    }
    printf("\nClient socket was created.\n\n");
    
    struct sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(PORT);
    serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
    
    int connectionStatus = connect(clientSocket, (struct sockaddr *) &serverAddress, sizeof(serverAddress));
    
    if (connectionStatus == -1) {
        printf("\nCan't making connection to the socket because of error: %s\n", strerror(errno));
        exit(1);
    }
    
}
