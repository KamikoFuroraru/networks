#include <errno.h>
#include <regex.h> 
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

#include "interaction.h"

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


char* getParent(char *input) { // example: /a/b -> /a
    char separator = '/';
    char *lastSeparator;
    int position;

    lastSeparator = strrchr(input, separator); // last separator in string
    position = lastSeparator - input;

    if (position != 0) input[position] = 0;
    return input;
}

char* getChild(char *input, char **paths, int size) {
    char regexp[SIZE];

    //expression => /path/\w+$
    strcpy(regexp, "\\");
    strcat(regexp, input);
    strcat(regexp, "\\/\\w+$");

    regex_t regex;
    int compiledRegex = regcomp(&regex, regexp, REG_EXTENDED);

    char path[SIZE];
    char *found;
    char *child;
    char result[SIZE];
    char *result_ptr = result;

    strcpy(result, "\0");
    strcpy(path, "\0");

    for (int i = 0; i < size; i++) {
        compiledRegex = regexec(&regex, paths[i], 0, NULL, 0);
        if (compiledRegex == 0) { // found reg exp

            strcpy(path, paths[i]);

            found = strtok(path, "/");
            child = found;

            while (1) {
                found = strtok(NULL, "/");
                if (found == NULL) break;
                else child = found;
            }

            strcat(result, child);
            strcat(result, "\n");

        }
    }

    return result_ptr;

}

int isRelative(char *input, char **paths, int size) {
    int result = -1;
    for (int i = 0; i < size; i++) {
        if (strcmp(input, paths[i]) == 0) {
            result = 1;
            break;
        }
        else result = 0;
    }
    return result;
}

int readN(int socket, char* buf, int size_msg) {
    int result = 0;
    int readBytes = 0;
    int size = size_msg;
    while(size > 0){
        readBytes = recv(socket, buf + result, size, 0);
        if (readBytes <= 0){
            return -1;
        }
        result += readBytes;
        size -= readBytes;
    }
    return result;
}