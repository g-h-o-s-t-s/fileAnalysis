#include <sys/types.h>
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>
#include <ctype.h>

#define MAX_WORDS 1000

static unsigned long int id[1000];    // global var for pthread
static pthread_mutex_t mut = PTHREAD_MUTEX_INITIALIZER;      // mutex

// function to check files and add to data structure (Linked List)
void* readFile(void* filePath) 
{
	FILE* file = fopen((char*) filePath, "r");  // opens the file
	
	int i, index = 0, isUnique, w;

	// List of distinct words
	char words[MAX_WORDS][100];
	char word[100];

	// Count of distinct words
	int count[MAX_WORDS];

	// Initialize words count to 0
	for (i = 0; i < MAX_WORDS; i++)
		count[i] = 0;

	if (file != NULL) 
	{
		while (fscanf(file, "%s", word) != EOF)
		{
			// Convert word to lowercase
			w = 0;
			while (word[w])
			{
				//start with letter
				if (isalpha(word[w]) || (word[w] == '-'))
				{
					word[w] = tolower(word[w]);
				}
				else
				{
					memmove(&word[w], &word[w + 1], strlen(word) - w);
				}
				w++;
			}

			// Check if word exits in list of all distinct words
			isUnique = 1;
			for (i = 0; i < index && isUnique; i++)
			{
				if (strcmp(words[i], word) == 0)
					isUnique = 0;
			}

			// If word is unique then add it to distinct words list
			// and increment index. Otherwise increment occurrence 
			// count of current word.
			if (isUnique) 
			{
				strcpy(words[index], word);
				count[index]++;

				index++;
			}
			else
			{
				count[i - 1]++;
			}
		}

		fclose(file);   // closes the file

		// Printing out the unique word list.
		printf("\nOccurrences of all distinct words in file: \n");
		for (i = 0; i < index; i++)
		{
			printf("%s    =>    %d\n", words[i], count[i]);
		}    
	
	}

	else
	{
		printf("ERROR: Could not open file.\n");
	}
	return NULL;
}

// function to check directories for files or sub-directories
void* readDirectory(void* target_path) 
{
	void* result;
	int i = 0;  

	struct dirent *entry;
	DIR *dir = opendir(target_path); 

	if (dir == NULL)  // opendir returns NULL if couldn't open directory 
	{
		printf("Invalid directory path.\n" );
	}
	else
	{
		// for readdir() 
		while ((entry = readdir(dir)) != NULL)
		{
			if (entry->d_type == DT_DIR 
				&& strcmp(entry->d_name, ".") != 0 
				&& strcmp(entry->d_name, "..") != 0)
			{
				pthread_mutex_lock(&mut);
				
				char* dir_name = malloc(strlen(target_path) + strlen(entry->d_name) + 2);
				strcpy(dir_name, target_path);
				strcat(dir_name, "/");
				strcat(dir_name, entry->d_name);
				printf("DIRECTORY: %s\n", dir_name);
				
				pthread_create(&id[i], NULL, &readDirectory, (void*) dir_name);
				i++;

				pthread_mutex_unlock(&mut);
			}
			else if (entry -> d_type == DT_REG)
			{
				pthread_mutex_lock(&mut);

				char* file_name = malloc(strlen(target_path) + strlen(entry->d_name) + 2);
				strcpy(file_name, target_path);
				strcat(file_name, "/");
				strcat(file_name, entry->d_name);
				printf("FILE: %s\n", file_name);

				pthread_create(&id[i], NULL, &readFile, (void*) file_name);
				i++;

				pthread_mutex_unlock(&mut);
			}
			else
			{
				printf("Invalid file type: %s\n", entry -> d_name);
			}
		}

		int j = 0;
		while (j < i)
		{
			pthread_join(id[j], &result);
			j++;
		}

		closedir(dir);
		free(result);
		pthread_exit(NULL);
	}
	return NULL;
}

int main(int argc, char** argv)
{
	if (argc != 2)
	{
		printf("ERROR: Invalid number of arguments passed.\n");
		return -1;
	}

	char* input = argv[1];
	
	readDirectory(input);

	pthread_mutex_destroy(&mut);
	return 0;
}
