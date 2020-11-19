#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

void listFiles(char *path);

int main()
{
    listFiles(".");
    return 0;
}


/**
 * Lists all files and sub-directories at given path.
 */
void listFiles(char *path)
{
    struct dirent *dp;
    DIR *dir = opendir(path);
    struct stat myfile;
    // Unable to open directory stream
    while ((dp = readdir(dir)) != NULL)
    {
         lstat(dp->d_name,&myfile);
         if (!dir)
                  return;
         else if (myfile.st_mode & 16384)  {
                  listFiles(dp->d_name);
         }

        else {
                  printf("%s\n", dp->d_name);
        }
    }
   // Close directory stream
    closedir(dir);
}


