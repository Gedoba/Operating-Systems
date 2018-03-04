if (NULL == (dir = opendir(".")))
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