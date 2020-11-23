
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>

int id[1000];

int main()
{
    DIR *dir;
    struct dirent *entry;
    if (!(dir = opendir(".")))
        return NULL;

    while ((entry = readdir(dir)) != NULL) {

        if (entry->d_type == DT_DIR) {
              dir_handling_func(entry->d_name);   }
        else if (entry->d_type == DT_REG) {
              file_handling_func(entry->d_name);  }
        else  {
              printf("Invalid file type\n");           }
return NULL;
} }


void *file_handling_func(void* path_to_file) {
    FILE  *file = fopen(path_to_file,"r");

    if(file != NULL) {
       mutex_lock();
       printf("%s",path_to_file);
       mutex_unlock();
    }
fclose(file);
}

void *dir_handling_func(void* target_path) {
    DIR *dir;
    struct dirent *entry;
    void *result;
    long  i = 0;
    opendir(target_path);
    while(readdir(target_path) != NULL) {
       if(target_path->d_type == DT_DIR) {
          pthread_create(&id[i], NULL, &dir_handling_func,NULL);
            i++;  }
       else {
          pthread_create(&id[i], NULL, &file_handling_func, NULL);
            i++;  }
     }
closedir(target_path);
}

