#ifndef NAVIGATION_H_
#define NAVIGATION_H_

char* getParent(char *input);
char* getChild(char *input, char **paths, int size);
int isRelative(char *input, char **paths, int size);
char* concat(const char *s1, const char *s2);

#define PATH_SIZE 100

#endif