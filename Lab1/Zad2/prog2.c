#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#define MAXL 20
int main(int argc, char** argv) {

    #define ERR(source) (perror(source),\
    fprintf(stderr,"%s:%d\n",__FILE__,__LINE__),\
    exit(EXIT_FAILURE))
         
    char name[MAXL+2];
    scanf("%s21" , name);
        if(strlen(name)>MAXL)
            ERR("Name too long");
    printf("Hello %s\n",name);
	    return EXIT_SUCCESS;
}