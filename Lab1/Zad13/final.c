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


void make_out(char* name, char* dirname);

int main(int argc, char** argv)
{
    char arrdirs [10][256]; //max dirs 10, 256 chars each
    int x = 0; //index of arrdirs
    DIR *dir;
    struct dirent  *dp;
    struct stat     statbuf;
    int c;
    while ((c = getopt (argc, argv, "p:o:")) != -1){
        switch(c){
            case 'p':            
            if (NULL == (dir = opendir(optarg)))
                printf("opendir failed\n");
            strcpy(arrdirs[x], optarg);
            /* Loop through directory entries. */
            while ((dp = readdir(dir)) != NULL) {
    
                /* Get entry's information. */
                if (lstat(dp->d_name, &statbuf) == -1)
                   continue;
                printf("Name of the object: \'%s\' ", dp->d_name);
                printf(" - size: %jdb\n", (intmax_t)statbuf.st_size);
                }
                closedir(dir);
            break;
            case 'o':
            make_out(optarg, arrdirs[0]);
            break;
            case '?':
            default:
            break;
        }
    x++; //next dir
    }
    return(0);
}

void make_out(char* name, char* dirname)
{
    DIR *dir;
    struct dirent  *dp;
    struct stat     statbuf;
    FILE* s1;

    if (NULL == (dir = opendir(dirname)))
        printf("opendir failed\n");
        /* Loop through directory entries. */
    while ((dp = readdir(dir)) != NULL){
        /* Get entry's information. */
        if (stat(dp->d_name, &statbuf) == -1)
            continue;
        if((s1=fopen(name,"a"))==NULL)
        printf("fopen failed\n");
        fprintf(s1,"Name of the object: \'%s\' ", dp->d_name);
        fprintf(s1, " - size: %jdb\n", (intmax_t)statbuf.st_size);
    }
    if(fclose(s1))
    printf("fclose failed\n");
    closedir(dir);
}