#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <regex.h> 
#include <libgen.h>

#include "server.h"
#include "createServer.h"
#include "interaction.h"


pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

struct Article {
	char title[SIZE];
	char author[SIZE];
	char text[SIZE];
	char infoPath[SIZE];
} *articles;

int clientCount = 0;    // number of clients online
int clientNumber = 0;   // last client id

int articleNumber = 0;

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

		else if(strcmp("exit", found) == 0) {            
			break;
		}

		else if (strcmp("kick-all", found) == 0) {
			if (clientCount == 0) {
				printf("\nThere is no one clients on the server.\n\n");
				continue;
			}
			kickAll();
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

	char msg[SIZE] = {"\0"};
	char curPath[SIZE] = "/animal";
	char art[SIZE] = {"\0"};
	char *found;
	int exist = 0;
	struct Article article;

	char regexp[SIZE];
	regex_t regex;
	int compiledRegex;


	while(1) {
		int recieve = readN(clientSocket, msg, sizeof(msg));
		if (recieve <= 0 || clientCount > BACKLOG) {
			kickClient(clientId);
			break;
		}

		found = strtok(msg, " ");

		if (found == NULL) {
			strcpy(msg, "takoi cmd net");
			send(clientSocket, msg, sizeof(msg), 0);
		}

		else if (strcmp("ls", found) == 0) {

			printf("\nClient %d send: %s\n\n", clientId, msg);

			strcpy(msg, "Current path: ");
			strcat(msg, curPath);
			strcat(msg, "\n\n");

			strcat(msg, "Next paths: \n");
			strcat(msg, getChild(curPath, paths, size));

			strcpy(art, "\n");
			strcat(art, "Articles: ");
			for (int i = 0; i < articleNumber; i++) {
				if (strcmp(articles[i].infoPath, curPath) == 0) {
					strcat(art, "\n");
					strcat(art, articles[i].title);
					exist++;
				}
			}

			if (exist > 0) strcat(msg, art);

			send(clientSocket, msg, sizeof(msg), 0);

			exist = 0;


		}

		else if (strcmp("cd", found) == 0) {
			found = strtok(NULL, " ");

			if (found == NULL) {
				strcpy(msg, "takoi cmd net");
				send(clientSocket, msg, sizeof(msg), 0);
			}

			else if (isRelative(found, paths, size) == 1) {
				printf("\nClient %d send: %s\n\n", clientId, msg);

				strcpy(curPath, found);

				strcpy(msg, "Current path: ");
				strcat(msg, curPath);
				strcat(msg, "\n");

				send(clientSocket, msg, sizeof(msg), 0);
			}

			else if (strcmp("..", found) == 0) {
				printf("\nClient %d send: %s\n\n", clientId, msg);

				strcpy(curPath, getParent(curPath));

				strcpy(msg, "Current path: ");
				strcat(msg, curPath);
				strcat(msg, "\n");

				send(clientSocket, msg, sizeof(msg), 0);
			}

			else {
				strcpy(regexp, found);
				strcat(regexp, "\n");
				compiledRegex = regcomp(&regex, regexp, REG_EXTENDED);
				compiledRegex = regexec(&regex, getChild(curPath, paths, size), 0, NULL, 0);

				if (compiledRegex == 0) {  // found reg exp
					strcat(curPath, "/");
					strcat(curPath, found);

					printf("%s\n", curPath);
					send(clientSocket, curPath, sizeof(curPath), 0);

				}

				else {
					strcpy(msg, "net takogo path");
					send(clientSocket, msg, sizeof(msg), 0);
				}
			}

		}

		else if (strcmp("upload", found) == 0) {
			send(clientSocket, msg, sizeof(msg), 0);

			readN(clientSocket, &article, sizeof(struct Article));
			articles = (struct Article*) realloc(articles, sizeof(struct Article) * (articleNumber + 1));
			articles[articleNumber] = article;
			strcpy(articles[articleNumber].infoPath, curPath);

			send(clientSocket, msg, sizeof(msg), 0);
			printf("\nUpload Succesfull\n");

			articleNumber++;

		}

		else if (strcmp("get", found) == 0) {
			send(clientSocket, msg, sizeof(msg), 0);
			readN(clientSocket, msg, sizeof(msg));

			for (int i = 0; i < articleNumber; i++) {
				if ((strcmp(articles[i].title, msg) == 0) &&  (strcmp(articles[i].infoPath, curPath) == 0)) {
					send(clientSocket, articles[i].text, sizeof(articles[i]. text), 0);
					exist++;
					break;
				}
			}

			if (exist == 0) {
				strcpy(msg, "statii s takim title net\n");
				send(clientSocket, msg, sizeof(msg), 0);
			}

			printf("\nGet to client Succesfull\n");
			exist = 0;

		}

		else if (strcmp("get-all", found) == 0) {
			send(clientSocket, msg, sizeof(msg), 0);
			readN(clientSocket, msg, sizeof(msg)); // read author name

			char author[SIZE];
			strcpy(author, msg);

			printf("author name = %s\n", author);

			for (int i = 0; i < articleNumber; i++) {
				if ((strcmp(articles[i].author, author) == 0) && (strcmp(articles[i].infoPath, curPath) == 0)) {
					exist++;
				}
			}

			sprintf(msg, "%d", exist);
			send(clientSocket, msg, sizeof(msg), 0); // send found count
			readN(clientSocket, msg, sizeof(msg));

			if (exist > 0) {
				for (int i = 0; i < articleNumber; i++) {
					if ((strcmp(articles[i].author, author) == 0) && (strcmp(articles[i].infoPath, curPath) == 0)) {
						send(clientSocket, articles[i].title, sizeof(articles[i].title), 0);
						readN(clientSocket, msg, sizeof(msg));
						send(clientSocket, articles[i].text, sizeof(articles[i].text), 0);
						readN(clientSocket, msg, sizeof(msg));
					}
				}

			}

			send(clientSocket, msg, sizeof(msg), 0);

			printf("\nGet-all Succesfull\n");
			exist = 0;

		}

		else {
			strcpy(msg, "takoi cmd net");
			send(clientSocket, msg, sizeof(msg), 0);
		}
	}
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