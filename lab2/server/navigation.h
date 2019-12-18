#ifndef NAVIGATION_H_
#define NAVIGATION_H_

char* getParent(char *input);
char* getChild(char *input, char **paths, int size);
int isRelative(char *input, char **paths, int size);

#define PATH_SIZE 1000

#endif