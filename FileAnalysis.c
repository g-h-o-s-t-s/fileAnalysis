#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>

int id[1000];

void *file_handling_func(char* path_to_file) {
    FILE  *file = fopen(path_to_file,"r");

    if(file != NULL) {
       static pthread_mutex_t mut = PTHREAD_MUTEX_INITIALIZER;
       pthread_mutex_lock(&mut);
       printf("%s",path_to_file);
       pthread_mutex_unlock(&mut);
    }
fclose(file);
}


void *dir_handling_func(char* target_path) {
    DIR *dir;
    struct dirent *entry;
    void *result;
    long unsigned  i = 0;
    opendir(target_path);
    while(entry = readdir(target_path) != NULL) {
       if(entry->d_type == DT_DIR) {
          pthread_create(&id[i], NULL, &dir_handling_func,NULL);
            i++;  }
       else {
          pthread_create(&id[i], NULL, &file_handling_func, NULL);
            i++;  }
     }
closedir(target_path);
}

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
    } 
    return NULL;
}
