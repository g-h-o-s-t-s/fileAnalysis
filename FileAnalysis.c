
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>

static unsigned long int id[1000];

void *file_handling_func(void *path_to_file) {
    FILE  *file = fopen((char*)path_to_file,"r");

    if(file != NULL) {
       static pthread_mutex_t mut = PTHREAD_MUTEX_INITIALIZER;
       pthread_mutex_lock(&mut);
       printf("%s",(char*)path_to_file);
       pthread_mutex_unlock(&mut);
    }
fclose(file);
}


void *dir_handling_func(void *target_path) {
    DIR *dir;
    struct dirent *entry;
    int i = 0;
    dir =  opendir((char*)target_path);
    while((entry = readdir(dir)) != NULL) {
       if(entry->d_type == DT_DIR) {
          pthread_create(&id[i], NULL, &dir_handling_func,NULL);
            i++;  }
       else {
          pthread_create(&id[i], NULL, &file_handling_func, NULL);
            i++;  }
     }
   int j=0;
   while((entry = readdir(dir)) != NULL)  {
         pthread_join(id[j],NULL);
         j++;   }
closedir(dir);
}


int main()
{
    DIR *dir;
    struct dirent *entry;
    if (!(dir = opendir(".")))
        return 0;

    while ((entry = readdir(dir)) != NULL) {

        if (entry->d_type == DT_DIR) {
              dir_handling_func(entry->d_name);   }
        else if (entry->d_type == DT_REG) {
              file_handling_func(entry->d_name);  }
        else  {
              printf("Invalid file type\n");           }
return 0;
closedir(dir);
  }
}
