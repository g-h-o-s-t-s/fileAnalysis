/*------------------------------------------------
	Sagnik Mukherjee and Treasa Bency
	November 29, 2020
------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <dirent.h>
#include <pthread.h>
#include <fcntl.h>

#include <math.h>
#include <ctype.h>
/*----------------------------------------------*/
#define RED printf("\033[0;31m")
#define YELLOW printf("\033[0;33m")
#define GREEN printf("\033[0;32m")
#define CYAN printf("\033[0;36m")
#define BLUE printf("\033[0;34m")
#define DEFAULT printf("\033[0m")
#define MAX_FILES 50
/*----------------------------------------------*/
struct meanNode
{
	char* name;
	float mean;
	struct meanNode* next;
};

struct tokenNode
{
	int count;
	char* tokenVal;
	float likelihood;
	struct tokenNode* next;
};

struct fileNode
{
	int totalWords;
	char* fileName;
	float klDiv;
	struct fileNode* next;
	struct tokenNode* tokenList;
};

struct threadNode 
{
	char* id;
	pthread_mutex_t* mewt;
	// Linked list of linked lists of tokenNodes.
	struct fileNode* fileList;
};

typedef struct meanNode meanNode;
typedef struct tokenNode tokenNode;
typedef struct fileNode fileNode;
typedef struct threadNode threadNode;
/*------------------------------------------*/
meanNode* createMean(char* name, float mean);
tokenNode* createToken(char* newToken);
fileNode* createFile(char* fileName);

void addToken(fileNode* fileHandle, char* token);
void readFrom(fileNode* currFile);
void switchTokens(tokenNode* one, tokenNode* two);
void switchFiles(fileNode* one, fileNode* two);

void* handleFile(void* filePath);
void* handleDirectory(void* target_path);

void colorize(float jsDist);
void updateTokenFreq(fileNode* file);

void freeMeans(meanNode* head);
void freeFiles(fileNode* head);

void printFiles(fileNode* head);
void printMeans(meanNode* head);

float calcKLDiv(fileNode* file, meanNode* mean);
void calcJSDist(fileNode* one, fileNode* two);
void fileComparator(fileNode* head);
/*------------------------------------------*/
meanNode* createMean(char* name, float mean)
{
	meanNode* newNode = (meanNode*) malloc(sizeof(meanNode));
	newNode->name = name;
	newNode->mean = mean;
	newNode->next = NULL;

	return newNode;
}

tokenNode* createToken(char* newToken)
{
	tokenNode* newNode = (tokenNode*) malloc(sizeof(tokenNode));
	char* string = (char*) malloc(sizeof(char)
						* strlen(newToken) + 1);
	strcpy(string, newToken);

	newNode->count = 1;
	newNode->tokenVal = string;
	newNode->likelihood = 0.0;
	newNode->next = NULL;

	return newNode;
}

fileNode* createFile(char* fileName)
{
	fileNode* newNode = (fileNode*) malloc(sizeof(fileNode));
	newNode->totalWords = 0;
	newNode->fileName = fileName;
	newNode->klDiv = 0.0;
	newNode->next = NULL;
	newNode->tokenList = NULL;

	return newNode;
}
/*----------------------------------------------*/
void addToken(fileNode* fileHandle, char* token)
{
	if (*token == 0 || isspace(*token))
		return;

	fileHandle->totalWords += 1;

	if (!fileHandle->tokenList)
		fileHandle->tokenList = createToken(token);

	else
	{
		tokenNode* curr = fileHandle->tokenList;
		while (curr->next)
		{
			// Increase count if duplicate token.
			if (!strcmp(curr->tokenVal, token))
			{
				curr->count += 1;
				return;
			}

			// Sort list when new token found.
			if (strcmp(curr->tokenVal, token) > 0)
			{
				tokenNode* newNode = createToken(token);
				switchTokens(curr, newNode);
				return;
			}
			curr = curr->next;
		}

		// Last token in list, so sort if needed.
		if (!curr->next)
		{
			if (!strcmp(curr->tokenVal, token))
				curr->count += 1;

			else if (strcmp(curr->tokenVal, token) > 0)
			{
				tokenNode* newNode = createToken(token);
				switchTokens(curr, newNode);
			}
			else
				curr->next = createToken(token);
		}
	}
}

// Token can be 100 characters plus null character;
// read file character by character to determine tokens.
void readFrom(fileNode* currFile)
{
	int fileDesc = open(currFile->fileName, O_RDONLY);
	if (fileDesc == -1)
	{
		printf("ERROR: Inaccessible file (%s).\n",
				currFile->fileName);
		pthread_exit(NULL);
	}

	int fileLen = lseek(fileDesc, 0, SEEK_END);
	char curr, string[101]; string[0] = '\0';
	int tokenLen = 0, index = 0, flag;
	
	// Reset file seeker to start of file.
	lseek(fileDesc, 0, SEEK_SET);
	do 
	{
		flag = read(fileDesc, &curr, 1);
		if (flag > 0)
		{
			index++;
			if (ispunct(curr) 
				&& (curr != '-') 
				&& (curr != '\''))
				continue;

			curr = tolower(curr);

			string[tokenLen] = curr;
			string[tokenLen + 1] = '\0';
			tokenLen++;

			// Whitespace acts as delimiter, end of token.
			if (index == fileLen 
				|| isdigit(curr) 
				|| isspace(curr))
			{
				if (isspace(curr) || isdigit(curr))
					string[tokenLen - 1] = '\0';

				addToken(currFile, string);
				
				// Reset vars for next token.
				tokenLen = 0;
				string[0] = '\0';
			}
		}
	} while (flag > 0);

	close(fileDesc);
}

// Dereference when storing temp value to switch.
void switchTokens(tokenNode* one, tokenNode* two)
{
	tokenNode temp = *one;
	*one = *two;
	*two = temp;
	one->next = two;
}

void switchFiles(fileNode* one, fileNode* two)
{
	fileNode temp = *one;
	*one = *two;
	*two = temp;
	one->next = two;
}
/*----------------------------------------------*/
// Receives files, swaps within fileList, 
// update thread structure.
void* handleFile(void* filePath)
{
	threadNode* threadList = (threadNode*) filePath;
	char* name = threadList->id;
	pthread_mutex_t* lock = threadList->mewt;
	fileNode* fileList = threadList->fileList;

	int id = open(name, O_RDONLY);
	if (id == -1) 
	{
		printf("ERROR: Inaccessible file (%s).\n", name);
		pthread_exit(NULL);
	}

	fileNode* currFile = createFile(name);
	readFrom(currFile);
	updateTokenFreq(currFile);

	pthread_mutex_lock(lock);

		if (!fileList->fileName)
		{
			// Dereference for first file in the list.
			*fileList = *currFile;
			free(currFile);
		} 

		else
		{
			fileNode* prev = fileList;
			while (prev->next)
			{
				if ((prev->totalWords) 
					> (currFile->totalWords))
				{
					switchFiles(prev, currFile);
					break;
				}
				prev = prev->next;
			}

			if (!prev->next)
			{
				if (prev->totalWords > currFile->totalWords)
					switchFiles(prev, currFile);
				else
					prev->next = currFile;
			}
		}
	pthread_mutex_unlock(lock);
	free(threadList);
	pthread_exit(NULL);
}

// Move through directories to access files/sub-directories.
void* handleDirectory(void* dirPath) 
{
	threadNode* threadList = (threadNode*) dirPath;
	char* name = threadList->id;
	pthread_mutex_t* lock = threadList->mewt;
	fileNode* fileList = threadList->fileList;
	free(dirPath);

	pthread_t id[MAX_FILES];
	int numThreads = 0;
	struct dirent* de;
	DIR* dePtr;

	if (!(dePtr = opendir(name)))
	{
		printf("ERROR: Inaccessible file (%s).\n", name);
		// Not currently in pthread, unlike handleFile 
		// would be at a similar point to this.
		exit(-1);
	}
	// Skip over current and parent directories.
	readdir(dePtr);	readdir(dePtr);

	while ((de = readdir(dePtr))) 
	{
		int nameLen = (strlen(name) + strlen(de->d_name) + 1);
		char* updatedName = (char*) malloc(sizeof(char) * nameLen);
		strcpy(updatedName, name);
		strcat(updatedName, de->d_name);

		threadNode* newThreadNode = (threadNode*) malloc(sizeof(threadNode));
		newThreadNode->fileList = fileList;
		newThreadNode->mewt = lock;

		if (de->d_type == DT_DIR)
		{
			strcat(updatedName, "/");
			newThreadNode->id = updatedName;
			pthread_create(&id[numThreads++], NULL,
				handleDirectory, (void*) newThreadNode);
		}

		else if (de->d_type == DT_REG)
		{
			newThreadNode->id = updatedName;
			pthread_create(&id[numThreads++], NULL,
				handleFile, (void*) newThreadNode);
		}
		else
			printf("ERROR: Unacceptable file type (%s).\n", de->d_name);
	}

	// Joining all currently spawned threads.
	int j;
	for (j = 0; j < numThreads; j++)
		pthread_join(id[j], NULL);

	closedir (dePtr);
	free(name);
	pthread_exit(NULL);
}
/*----------------------------------------------*/
// Toggle console text color of Jensen-Shannon Distance.
void colorize(float jsDist)
{
	if (jsDist >= 0 && jsDist <= 0.10)
	{
		RED;
		printf("%f ", jsDist);
		DEFAULT;
	}
	else if (jsDist > 0.1 && jsDist <= 0.15)
	{
		YELLOW;
		printf("%f ", jsDist);
		DEFAULT;
	}
	else if (jsDist > 0.15 && jsDist <= 0.20)
	{
		GREEN;
		printf("%f ", jsDist);
		DEFAULT;
	}
	else if (jsDist > 0.2 && jsDist <= 0.25)
	{
		CYAN;
		printf("%f ", jsDist);
		DEFAULT;
	}
	else if (jsDist > 0.25 && jsDist <= 0.3)
	{
		BLUE;
		printf("%f ", jsDist);
		DEFAULT;
	}
	else if (jsDist > 0.30)
	{
		if (jsDist > 0.30103)
			printf("0.30103");
		else
			printf("%f ", jsDist);
	}
}

// Determines likelihood of token appearing within a file.
void updateTokenFreq(fileNode* currentFile)
{
	tokenNode* curr = currentFile->tokenList;
	while (curr)
	{
		curr->likelihood = ((float) curr->count 
			/ (float) currentFile->totalWords);
		curr = curr->next;
	}
}
/*----------------------------------------------*/
// Free memory reserved for a list of means.
void freeMeans(meanNode* head)
{
	meanNode* curr = head;
	while (curr)
	{
		meanNode* prev = curr;
		curr = curr->next;
		free(prev);
	}
}

// Free fileList, as well as each file's tokenList.
void freeFiles(fileNode* head)
{
	// Start off at head of passed fileList.
	fileNode* currF = head;

	while (currF)
	{
		tokenNode* currT = currF->tokenList;
		// Free up the current tokenList.
		while (currT)
		{
			tokenNode* prevW = currT;
			currT = currT->next;
			free(prevW->tokenVal);
			free(prevW);
		}
		free(currT);

		// Free current file pointer.
		fileNode* prevF = currF;
		currF = currF->next;
		free(prevF->fileName);
		free(prevF);
	}
}
/*----------------------------------------------*/
// Debug function.
void printFiles(fileNode* head)
{   
	fileNode* temp = head;

	while (temp)
	{
		printf("(%s) Unique Tokens: %d\n", 
				temp->fileName, temp->totalWords);
		// Traversing the list of tokens.
		tokenNode* curr = temp->tokenList;
		while (curr)
		{
			printf("(%s) Frequency:%d     Percentage:%f\n",
				curr->tokenVal, curr->count, curr->likelihood);
			curr = curr->next;
		}
		temp = temp->next;
	}
}

// Debug function.
void printMeans(meanNode* head)
{
	meanNode* temp = head;
		while (temp)
		{
			printf("(%s)  Mean:%f.\n", temp->name, temp->mean);
			temp = temp->next;   
		}
}
/*----------------------------------------------*/
// Return KL-Divergence.
float calcKLDiv(fileNode* file, meanNode* mean)
{
	tokenNode* tokenList = file->tokenList;
	meanNode* meanList = mean;
	float klDiv = 0.0;

	// At each node in meanList, determine likelihood of token appearing.
	while (meanList)
	{
		while (tokenList)
		{
			if (!strcmp(tokenList->tokenVal, meanList->name))
			{
				klDiv += tokenList->likelihood
					* log10(tokenList->likelihood
						/ meanList->mean);
				break;
			}
			tokenList = tokenList->next;
		}
		// Proceed to next node in meanList, reset tokenList pointer.
		tokenList = file->tokenList;
		meanList = meanList->next;
	}

	return klDiv;
}

// Determine Jensen-Shannon Distance between two files.
// Final console output generated here.
void calcJSDist(fileNode* one, fileNode* two)
{
	tokenNode* tokens1 = one->tokenList;
	tokenNode* tokens2 = two->tokenList;

	// BASECASE 1:
	// If the second file's tokenList is NULL,
	// the first file's tokenList is assumed NULL,
	// and the JS-distance is zero.
	if (!tokens2)
	{
		colorize(0.0);
		printf(" \"%s\" and \"%s\"\n", one->fileName, two->fileName);
		return;
	}

	// BASECASE 2:
	// If second file tokenList exists, generate meanList for
	// tokens2 alone, and determine JS-distance.
	if (!tokens1 && tokens2)
	{
		meanNode* meanList = createMean(tokens2->tokenVal, 
								(tokens2->likelihood) / 2.0);
		meanNode* currMean = meanList;

		if (tokens2->next)
			tokens2 = tokens2->next;

		while (tokens2)
		{
			currMean->next = createMean(tokens2->tokenVal, 
								(tokens2->likelihood) / 2.0);
			tokens2 = tokens2->next;
			currMean = currMean->next;
		}

		float a = calcKLDiv(one, meanList);
		float b = calcKLDiv(two, meanList);
		float jsDist = (a + b) / 2.0;

		colorize(jsDist);
		printf(" \"%s\" and \"%s\"\n", one->fileName, two->fileName);
		printf("---------------------\n");
		printMeans(meanList);
		freeMeans(meanList);
		return;
	}

	// Generate first meanNode, populate meanList.
	meanNode* meanList = createMean(tokens1->tokenVal, 
							(tokens1->likelihood) / 2.0);
	meanNode* currMean = meanList;

	if (tokens1->next)
		tokens1 = tokens1->next;

	while (tokens1)
	{
		currMean->next = createMean(tokens1->tokenVal,
							(tokens1->likelihood) / 2.0);
		tokens1 = tokens1->next;
		currMean = currMean->next;
	}
	
	// Reset mean pointer to head of list.
	currMean = meanList;
	
	// Update means respective to tokens2.
	while (currMean->next)
	{
		if (!strcmp(tokens2->tokenVal, currMean->name))
		{
			currMean->mean += (tokens2->likelihood / 2.0);
			break;
		}
		currMean = currMean->next;
	}

	if (!currMean->next)
		currMean->next = createMean(tokens2->tokenVal, 
							(tokens2->likelihood) / 2.0);

	// Move to next node in tokens2, determine if
	// currT is present in meanList.
	tokens2 = tokens2->next;
	while (tokens2)
	{
		char* currT = tokens2->tokenVal;
		while (currMean->next)
		{
			if (!strcmp(currT, currMean->name))
			{
				// Token matched.
				currMean->mean += (tokens2->likelihood / 2.0);
				break;
			}
			currMean = currMean->next;
		}
		
		// Check the last node in the list.
		if (currMean->next)
		{
			if (!strcmp(currT, currMean->name))
			{
				// Token matched.
				currMean->mean += (tokens2->likelihood / 2.0);
				break;
			}
			// List ended, generate new meanNode.
			else 
				currMean->next = createMean(currT, 
									(tokens2->likelihood) / 2.0);
		}
		tokens2 = tokens2->next;
	}

	float a = calcKLDiv(one, meanList);
	float b = calcKLDiv(two, meanList);
	float jsDist = (a + b) / 2.0;

	colorize(jsDist);
	printf(" \"%s\" and \"%s\"\n", one->fileName, two->fileName);
	printf("---------------------\n");
	printMeans(meanList);
	freeMeans(meanList);
}

// After reading in files, compare file pairs in fileList.
void fileComparator(fileNode* head)
{
	// List must have more than one file for comparison.
	if (!head || !head->next)
	{
		printf("ERROR: Multiple files required for comparison.\n");
		return;
	}

	// Set pointers to first two files in list.
	fileNode* one = head;
	fileNode* two = head->next;
	while (one)
	{
		while (two)
		{
			calcJSDist(one, two);
			two = two->next;
		}

		// Attempt to point to next pair of files
		// in fileList for comparison.
		one = one->next;
		if (one->next)
			two = one->next;
		else 
			break;
	}
}
/*----------------------------------------------*/
// Driver function.
int main(int argc, char* argv[])
{
	if (argc != 2)
	{
		printf("ERROR: Directory pathname not provided.\n");
		return -1;
	}

	// Populate the fileList.
	fileNode* fileList = malloc(sizeof(fileNode));
	fileList->fileName = NULL;

	// Initialize mutex var.
	pthread_mutex_t* lock = (pthread_mutex_t*) malloc(sizeof(pthread_mutex_t));
	pthread_mutex_init(lock, NULL);

	// Generate starting directory pathname.
	char* initPath = (char*) malloc(sizeof(char) * strlen(argv[1]) + 2);
	strcpy(initPath, argv[1]);
	strcat(initPath, "/");

	// Allocate memory for global threads struct; initialize
	// struct members.
	threadNode* threadList = (threadNode*) malloc(sizeof(threadNode));
	threadList->id = initPath;
	threadList->mewt = lock;
	threadList->fileList = fileList;

	// Spawn overall thread, read through directory,
	// and any sub-directories and/or files.
	pthread_t programThread;
	pthread_create(&programThread, NULL, handleDirectory, (void*) threadList);
	pthread_join(programThread, NULL);

	// Report file similarity findings.
	fileComparator(fileList);
	printf("---------------------\n");
	printFiles(fileList);

	// Free memory resources.
	freeFiles(fileList);
	free(lock);

	return 0;
}
