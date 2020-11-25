#include <sys/types.h>
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>

static unsigned long int id[1000];      // global var for pthread
static pthread_mutex_t mut = PTHREAD_MUTEX_INITIALIZER;    // mutex

// function to check files and add to data structure (Linked List)
void *file_handling_func(void *path_to_file) {
    FILE  *file = fopen((char*)path_to_file,"r");  // opens the file

    if(file != NULL) {
       pthread_mutex_lock(&mut);
       printf("%s",(char*)path_to_file);   // add to data struct (for now just prints filename)
       pthread_mutex_unlock(&mut);
    }
fclose(file);   // closes the file
}

// function to check directories for files or sub-directories
void *dir_handling_func(void *target_path) {
    DIR *dir;
    struct dirent *entry;
    int i = 0;
    void *result;
    dir =  opendir(target_path);
    entry = readdir(dir);
    if(dir != NULL) {
     while(entry != NULL) {
       // if the directory has sub-directories. Avoid "." and ".." directories
       if(entry->d_type == DT_DIR && strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
          char* dir_name = malloc(strlen(target_path) + strlen(entry->d_name) +2);
          strcpy(dir_name, target_path);
          strcat(dir_name, "/");
          strcat(dir_name, entry->d_name);
          // create pthread for the sub-directory
          pthread_create(&id[i], NULL, &dir_handling_func,(void*)dir_name);
            i++;  }
       // if the directory has files
       else {
         char* file_name = malloc(strlen(target_path) + strlen(entry->d_name) + 2);
         strcpy(file_name, target_path);
         strcat(file_name, "/");
         strcat(file_name, entry->d_name);

         printf("%s\n", file_name);
         // create pthread for the file
         pthread_create(&id[i], NULL, &file_handling_func,(void*)file_name);
         i++;  }
     }
     int j = 0;
     // join all threads
     while( j < i )  {
         pthread_join(id[j],&result);
         j++;   }
         closedir(dir);
         free(result);
         pthread_exit(NULL);
     }
    else{
         printf("ERROR: Invalid pathway\n");
    }
  return NULL;
}

int main()
{
    DIR *dir;
    struct dirent *entry;
    if ((dir = opendir(".")) != NULL)
    while ((entry = readdir(dir)) != NULL) {
        // checks if entry is a directory
        if (entry->d_type == DT_DIR && strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
              dir_handling_func((void*)entry->d_name);   }
        // checks if entry is a regular file
        else if (entry->d_type == DT_REG) {
              file_handling_func((void*)entry->d_name);  }
        // returns an error for other files
        else  {
              printf("ERROR: Invalid file type\n");      }
     }
    closedir(dir);
    return 0;
}
