#include <string.h>
#include <stdio.h>
#include <stdlib.h>
void usage(char* pname){
    fprintf(stderr,"USAGE:%s name times>0\n",pname);
    exit(EXIT_FAILURE);
}
int main(int argc, char** argv) { 
    int i;
        if(argc>3)
        usage(argv[0]);

	for(i=0;i<atoi(argv[2]);i++)
        printf("Hello ""%s\n",argv[1]);
        return EXIT_SUCCESS;

}