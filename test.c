#include <stdio.h>
#include <string.h>
#include <regex.h> 
#include <stdlib.h>

#define SIZE 100

char* concat(const char *s1, const char *s2);
void getParent(char *input);
void getChild(char *input);

const char *paths[] = {"/animal", "/animal/cat", "/animal/dog", "/animal/cat/big", "/animal/cat/small", "/animal/dog/big", "/animal/dog/small", "/animal/cat/big/leopard", "/animal/cat/big/panther", "/animal/cat/small/home"};
int size = sizeof(paths)/sizeof(paths[0]);

int main(int argc, char** argv) {

	char input[SIZE];
	fgets(input, sizeof(input), stdin);
	input[strlen(input) - 1] = '\0';

	getChild(input);
    getParent(input);   

	return 0;
}

void getParent(char *input) { // example: /a/b -> /a
	char separator = '/';
    char *lastSeparator;
    int position;

    lastSeparator = strrchr(input, separator); // last separator in string

    if (lastSeparator == NULL)
    	printf("%s\n", "Nothing");
    else
    	position = lastSeparator - input;

    char pathWithoutLast[SIZE];
    strncpy(pathWithoutLast, input, position); // path without last separator

    if (strlen(pathWithoutLast) == 0)
    	printf("cmd: cd .. :  %s   d %ld\n", input, sizeof(input));
    else 
    	printf("cmd: cd .. :  %s\n", pathWithoutLast);
}

void getChild(char *input) {
	char *regexp;
    char *allRegexp;

    //expression => /path/\w+$
    regexp = concat("\\", input);
    allRegexp = concat(regexp, "\\/\\w+$");

	regex_t regex;
    int compiledRegex = regcomp(&regex, allRegexp, REG_EXTENDED);

    char path[SIZE];
    char *found;
	char *result;

    for (int i = 0; i < size; i++) {
		compiledRegex = regexec(&regex, paths[i], 0, NULL, 0);
		if (compiledRegex == 0) { // found reg exp
        	//printf("%s\n", paths[i]); //   /a/b

			strcpy(path, paths[i]);

			found = strtok(path, "/");
			result = found;

			while (1) {
				found = strtok(NULL, "/");
				if (found == NULL) break;
				else result = found;
			}
			printf("cmd: ls : %s\n", result); // b
        }
        //else printf("Regex not found\n");
    }

}

char* concat(const char *s1, const char *s2) {
    char *result = malloc(strlen(s1) + strlen(s2));
    strcpy(result, s1);
    strcat(result, s2);
    return result;
}
