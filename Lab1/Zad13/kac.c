#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>
#include <dirent.h>
#include <sys/types.h>
#include <fcntl.h>

#define MAX_PATH 30

#define ERR(source) (perror(source),\
fprintf(stderr,"%s:%d\n",__FILE__,__LINE__),\
exit(EXIT_FAILURE))

void usage(char* pname){
	fprintf(stderr,"USAGE:%s -n Name -p OCTAL -s SIZE\n",pname);
	exit(EXIT_FAILURE);
}

void list_dir(char* file){
    FILE *s1;
    DIR *dirp;
    struct dirent *dp;
    struct stat filestat;

    if(NULL == (dirp = opendir(".")))
        ERR("opendir");
    if((s1 = fopen(file,"w+")) == NULL) 
        ERR("fopen");
    if(file != NULL)
        fprintf(s1,"LISTA PLIKÓW:\n");
    else 
        printf("LISTA PLIKÓW:\n"); 

    while((dp = readdir(dirp))!= NULL){
        if(lstat(dp->d_name,&filestat))
            printf("lstat\n");
        if(file != NULL)
            fprintf(s1,"%s %jd\n", dp->d_name,filestat.st_size);        
        else
            printf("%s %jd\n", dp->d_name,filestat.st_size);
        }
    if(fclose(s1))
        ERR("fclose");
}

int main(int argc, char** argv) {
    char path[MAX_PATH];
    if(getcwd(path,MAX_PATH)==NULL)
        ERR("getcwd");
    int iso = 0;
    char c;
    char** dirlist = malloc(MAX_PATH);
    int i = 0;
    char* file = NULL; 
    while ((c = getopt (argc, argv, "p:o:")) != -1){
        switch (c){
            case 'p':
                dirlist = realloc(dirlist,(i+1)*MAX_PATH);
                dirlist[i] = optarg;
                i++;
                break;
            case 'o':
                iso++;
                if(iso > 1)
                    usage(argv[0]);
                file = optarg;
                break;
            case '?':
            default:
                usage(argv[0]);
        }
    }

    for(int j = 0;j<i;j++){
        if(chdir(dirlist[j])) ERR("chdir");
        if(file != NULL)
        printf("SCIEŻKA:\n%s\n",dirlist[j]);
        list_dir(file);
        if(chdir(path)) ERR("chdir");
    }
return EXIT_SUCCESS;
}