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

int main(int argc, char** argv)
{
    DIR *dir;
    struct dirent  *dp;
    struct stat     statbuf;
    int c;
    //char* optarg = NULL;
    while ((c = getopt (argc, argv, "p:")) != -1){
        switch(c){
            case 'p':
            //printf("%s", optarg);
            if (NULL == (dir = opendir(optarg)))
                printf("opendir failed\n");
    
            /* Loop through directory entries. */
            while ((dp = readdir(dir)) != NULL) {
    
                /* Get entry's information. */
                if (stat(dp->d_name, &statbuf) == -1)
                   continue;
                printf("Name of the object: \'%s\' ", dp->d_name);
                printf(" - size: %jdb\n", (intmax_t)statbuf.st_size);
                }
                closedir(dir);
            break;
            case 'o':

            case '?':
            default:
            break;
        }
    }
    return(0);
    


}