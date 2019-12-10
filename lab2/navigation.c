#include <stdio.h>
#include <string.h>
#include <regex.h> 
#include <stdlib.h>

#include "navigation.h"
/*
int main(int argc, char** argv) {
    char *paths[] = {"/animal", "/animal/cat", "/animal/dog", "/animal/cat/big", "/animal/cat/small", "/animal/dog/big", "/animal/dog/small", "/animal/cat/big/leopard", "/animal/cat/big/panther", "/animal/cat/small/home"};
    int size = sizeof(paths) / sizeof(paths[0]); // SIZE OF MASSIVE OF PATHS
    char input[PATH_SIZE] = "/animal/cat/big";
    printf("%s\n", input);
    if (isRelative(input, paths, size) == 1) {
        printf("%s\n", "est takoe");
        char* child = getChild(input, paths, size);
        if (child != NULL)
            printf("%s", child);
        char* parent = getParent(input);
        if (parent != NULL)
            printf("%s\n", parent);
    }
    else 
        printf("%s\n", "net takogo");
    return 0;
} */

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
    char *regexp;

    //expression => /path/\w+$
    regexp = concat("\\", input);
    regexp = concat(regexp, "\\/\\w+$");

    regex_t regex;
    int compiledRegex = regcomp(&regex, regexp, REG_EXTENDED);

    char path[PATH_SIZE] = {"\0"};
    char *found;
    char *child;
    char *result = {"\0"};

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

            result = concat(result, child);
            result = concat(result, "\n");

        }
        //else printf("Regex not found\n");
    }

    return result;

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

char* concat(const char *s1, const char *s2) {
    char *result = malloc(strlen(s1) + strlen(s2));
    strcpy(result, s1);
    strcat(result, s2);
    return result;
}