#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>


static unsigned long int id[1000];    // global var for pthread
static pthread_mutex_t mut = PTHREAD_MUTEX_INITIALIZER;      // mutex

// function to handle files and add to struct 
void *file_handling_func(void *path_to_file) {
    FILE  *file = fopen(path_to_file,"r");

    if(file != NULL) {
       pthread_mutex_lock(&mut);
       printf("%s\n",(char*)path_to_file); // add to struct (for now prints file name)
       pthread_mutex_unlock(&mut);
    }
fclose(file);
}

// function to handle directories with subdirectories and files
void *dir_handling_func(void *target_path) {
    DIR *dir;
    struct dirent *entry;
    int i = 0;
    void *result;
    dir =  opendir(target_path);
    if(dir != NULL) {
    while((entry = readdir(dir)) != NULL) {
        // if the entry is a directory
       if(entry->d_type == DT_DIR && strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
          char* dir_name = malloc(strlen(target_path) + strlen(entry->d_name) +2);
          strcpy(dir_name, target_path);
          strcat(dir_name, "/");
          strcat(dir_name, entry->d_name);
          pthread_create(&id[i], NULL, &dir_handling_func,(void*)dir_name);
          i++;  }
        // if entry is a file
       else if(entry -> d_type == DT_REG)  {
          char* file_name = malloc(strlen(target_path) + strlen(entry->d_name) + 2);
          strcpy(file_name, target_path);
          strcat(file_name, "/");
          strcat(file_name, entry->d_name);
          pthread_create(&id[i], NULL, &file_handling_func,(void*)file_name);
          i++;  }
        // if file is nnot a regular file, marks it invalid
       else {
          printf("Invalid file type: %s\n", entry -> d_name);     }
     }
   // joins the pthreads
   int j=0;
   while(j<i)  {
         pthread_join(id[j],&result);
         j++;   }
   closedir(dir);
   free(result);
   pthread_exit(NULL);
   }
   // if directory is invalid
   else {
        printf("ERROR: file pathway is not found.\n");
   }
}

int main()
{
    DIR *dir;
    struct dirent *entry;
    if (!(dir = opendir(".")))
        return 0;

    while ((entry = readdir(dir)) != NULL) {

        if (entry->d_type == DT_DIR ) {
              dir_handling_func(entry->d_name);   }
        else if (entry->d_type == DT_REG) {
              file_handling_func(entry->d_name);  }
        else  {
              printf("Invalid file type(main): %s\n",entry->d_name);           }
  }
return 0;
closedir(dir);
}
