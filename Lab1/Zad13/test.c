#include <stdio.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>


#define ERR(source) (perror(source),\
fprintf(stderr,"%s:%d\n",__FILE__,__LINE__),\
exit(EXIT_FAILURE))

void scan_dir();

int main(int argc, char **argv)
{
    char* name = "temp";
    char c;
    while ((c = (getopt(argc, argv, "p:o:")))!= -1){
        switch(c){
            case 'p':
                chdir(optarg);
                printf("DIRECTORY: %s \n", optarg);
                scan_dir(name);
                printf("dupa\n");
                break;
            case 'o':
                printf("optarg");
                name = optarg;
                break;
        }
    }
    printf("dupa");
//    rename("/home/karol/Desktop/temp", strcat("/home/karol/Desktop/",name));
    return EXIT_SUCCESS;
}

void scan_dir(char* name)
{
    FILE* s1;
    DIR *d;
    struct dirent *dp;
    struct stat filestat;
    if((s1=fopen("/home/tomek/SOP/Lab1/test","a"))==NULL)
        ERR("fopen");
    if ((d = opendir(".")) == NULL)
    {
        fprintf(stderr, "Cannot open ./ directory\n");
        exit(1);
    }
    while ((dp = readdir(d)) != NULL)
    {
        lstat(dp->d_name, &filestat);
        fprintf(s1, "%s  %ld \n", dp->d_name, filestat.st_size);
    }
    closedir(d);
    fclose(s1);
    printf("koniec");
}