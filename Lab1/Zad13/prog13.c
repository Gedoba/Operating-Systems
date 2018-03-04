#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <locale.h>
#include <langinfo.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define MAX_PATH 30
void list_dir(char* name);

int main(int argc, char** argv)
{
    char path [MAX_PATH];
    if(getcwd(path, MAX_PATH)==NULL)
        printf("getcwd failed\n");
    int iso = 0;
    char** dirlist = malloc(MAX_PATH);
    char* file = NULL;
    int i = 0;
    int c;
    while (((c = getopt (argc, argv, "p:o:")) != -1)){
        switch(c){
            case 'p':
                dirlist = realloc(dirlist, (i+1)*MAX_PATH);
                dirlist[i] = optarg;
                i++;
            break;
            case 'o':
            iso++;
            if(iso>1)
                exit(EXIT_FAILURE);
            file = optarg;
            break;
            case '?':;
        }
    }
    for(int j = 0; j<i; j++){
        if(chdir(dirlist[j]))
            printf("chdir failed\n");
        if(file != NULL)
            printf("ŚCIEŻKA:\n %s\n", dirlist[j]);
        list_dir(file);
        if(chdir(path))
            printf("chdir failed\n");
}
    return EXIT_SUCCESS;
}

void list_dir(char* name)
{
    DIR *dir;
    struct dirent  *dp;
    struct stat filestat;
    FILE* s1;

    if (NULL == (dir = opendir(".")))
        printf("opendir failed\n");
    if((s1 = fopen(name, "w+")) == NULL)
        printf("fopen failed\n");
    if(NULL != name)
        fprintf(s1, "LISTA PLIKÓW:\n");
    else
        printf("LISTA PLIKÓW:\n");

        /* Loop through directory entries. */
    while ((dp = readdir(dir)) != NULL){
        /* Get entry's information. */
        if (lstat(dp->d_name, &filestat) == -1)
            printf("lstat failed\n");
        if(name != NULL)
            fprintf(s1,"Name: \'%s\' size: %jdb\n", dp->d_name,filestat.st_size);
        else
            printf("Name: \'%s\' size: %jdb\n", dp->d_name,filestat.st_size);
    }
    if(fclose(s1))
        printf("fclose failed\n");
    closedir(dir);
}