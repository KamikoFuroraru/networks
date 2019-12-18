#ifndef INTERACTION_H_
#define INTERACTION_H_

void* checkSocket(int* exp, char* msg);
void* checkThread(int thread, char* msg);
int readN(int socket, char* buf, int size_msg);
char* getParent(char *input);
char* getChild(char *input, char **paths, int size);
int isRelative(char *input, char **paths, int size);

#define SIZE 1000

#endif