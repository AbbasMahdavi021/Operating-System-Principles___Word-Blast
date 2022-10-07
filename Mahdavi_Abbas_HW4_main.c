/***********************************************************
* Class:  CSC-415-03 Fall 2022
* Name: Abbas Mahdavi
* Student ID: 918345420
* GitHub Name: AbbasMahdavi021
* Project: Assignment 4 – Word Blast
*
* File: Mahdavi_Abbas_HW4_main.c
*
* Description: 
* This program is to read War and Peace 
* (a text copy is included with this assignment) 
* and it is to count and tally each of the words 
* that are 6 or more characters long.
*
**************************************************************/
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <strings.h>
#define wordLength 6

// You may find this Useful
char * delim = "\"\'.“”‘’?:;-,—*($%)! \t\n\x0A\r";

// Initialize the lenght of the wordList
volatile int listLength = 0; 

/*define a structure and init a struct array
  for word and the frequecy of words
  to be used in the main function.*/
typedef struct Word {
    //the word in the file
    char* word;
    //how many time that word appears           
    int frequency; 
} Word;


/*define a struct and variables needed for
  thread creation.*/
typedef struct ThreadArgs {   
    Word* wordList;
    int threadID;
    int fileID;
    int blockSize;
} ThreadArgs;


/*creating two pthread mutexes, one for adding the wor to list
the other for increamenting the word in the list */
pthread_mutex_t addMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t incrementMutex= PTHREAD_MUTEX_INITIALIZER;



//Helper Function for processing the words
void wordProcess(Word* wordList, char* word){
    /*check if the word already exists int the list,
    if so increment to account for the count. */
    for(int i=0; i< listLength; ++i){
        //simple way around null issue
        if(wordList[i].word==NULL || word==NULL){
            continue;
        }
        //When the word is found, increment the frequency
        if(strcasecmp(wordList[i].word, word) == 0 ){
            //lock
            pthread_mutex_lock(&incrementMutex);
            wordList[i].frequency++;
            //free
            pthread_mutex_unlock(&incrementMutex);
          
            return;
        }
    }
    //adding the new found word, to the list for the first time
    pthread_mutex_lock( &addMutex );
    //list size increases
    listLength++;
    //first appearance, frequency/count of 1
    wordList[listLength-1].frequency=1;

    wordList[listLength-1].word = malloc(strlen(word)+1);
    if(wordList[listLength-1].word == NULL){
        printf("malloc Error Occured!");
        //exit is error found!
        exit(-1);
    }
    //strcpy the last work in the list
    strcpy(wordList[listLength-1].word, word);
    pthread_mutex_unlock( &addMutex );
}

//Helper Function for processing the file
void* fileProcess( void* args ) {
    //init a ThreadArgs pointer
    ThreadArgs *line = (ThreadArgs*) args;

    char threadBuffer[line->blockSize + 1];
    //using pread to allocate size of file into thread's buffer
    //https://linux.die.net/man/2/pread

    if (pread( line->fileID, 
               threadBuffer, sizeof(threadBuffer), 
               (line->threadID) * line->blockSize) == -1) {
        
        printf( "preadProcess Failed!");
    }
    //must null terminatie the buffer
    threadBuffer[line->blockSize] = '\0'; 
    
    char* p=NULL;
    // We can tokenize the words, with given delims
    char* token = strtok_r( threadBuffer, delim, &p );
    //tokenize the portion of the thread
    while (token!=NULL) {
        /*If the word has longer than 6,
        we take account of it by wordProccess function,
        and do the same with the next word. */
        if (strlen( token ) >= wordLength ) {
           wordProcess(line->wordList, token);
        }
        //next word
        token = strtok_r( NULL, delim, &p );
    }
}   


int main (int argc, char *argv[]){
    
    //check for proper cmd line argument
    if (argc != 3) {
        fprintf( stderr, "Improper commandline arguments\n" );
        return -1;
    }

    //text file and number of threads vars
    const char* fileName = argv[1];
    const int threadCount = atoi( argv[2] );

    //get and init the file
    int fileID = open( fileName, O_RDONLY ); 
    if (fileID == -1) {
        perror( "Could not reach file" );
        return -1;
    }
    
    //take account for the total size of the file
    struct stat s;
    if (fstat( fileID, &s ) == -1) {
        perror( "Error!" );
        exit( EXIT_FAILURE );
    }
    //set the size of the file in bytes to a varaible for easy usage.
    size_t fileSize = s.st_size;
    

    //Making sure the size of file matches the size of the list
    Word* wordList = malloc( fileSize * sizeof( Word ) );
    if (wordList == NULL) {
        fprintf( stderr, "Could not create list of words!" );
        return -1;
    }

    /*dividing the size of the file, by amount of threads using,
    to the the proper size of each block*/
    int blockSize = fileSize / threadCount;

    //**************************************************************
    // DO NOT CHANGE THIS BLOCK
    //Time stamp start
    struct timespec startTime;
    struct timespec endTime;

    clock_gettime(CLOCK_REALTIME, &startTime);
    //**************************************************************
    // *** TO DO ***  start your thread processing
    //                wait for the threads to finish
    
    //create an array of threads, based on the number of threads
    pthread_t threads[threadCount];

    /*initialize the function arguments, 
      and set the corresponding variables
      to correct position in argsList */
    ThreadArgs argsList [threadCount];
    for (int i=0; i< threadCount; ++i){
        argsList[i].wordList = wordList;
        argsList[i].threadID = i;
        argsList[i].fileID = fileID;
        argsList[i].blockSize = blockSize;
    }

    //process the threads in the array, and use fileProcess Funtions
    //keep result in a variable
    int result;
    for (int i = 0; i < threadCount; ++i) {
        result = pthread_create( &threads[i], NULL, 
                 fileProcess, (void*) &argsList[i] );
        if (result != 0) {
            //if res is not zero, thread creations failed.
            fprintf( stderr, "Error in Thread creation!");
            exit(1);
        }
        //bring back all threads to the main thread
    }   
    for(int i=0; i< threadCount; i++){
        pthread_join( threads[i], NULL );
    }
    
    //Get the top 10 words with min 6 length, and 
    // print their count properly into the console
    Word topWord;
    for (int x = 0; x < listLength; x++)
    {
        for (int y = x + 1; y < listLength; y++)
        {
            if (wordList[x].frequency < wordList[y].frequency)
            {
                topWord = wordList[x];
                wordList[x] = wordList[y];
                wordList[y] = topWord;
            }
        }
        //with single thread, time would differ
    }   if (threadCount == 1){
            printf("\nWord Frequency Count on %s with %d thread\n",
             argv[1], threadCount);
        //other threads would have same time
    }   else if (threadCount != 1){
            printf("\nWord Frequency Count on %s with %d threads\n",
             argv[1], threadCount);
    }   
    printf("Printing top 10 words 6 characters or more.\n");
        
    //print the word and its number of frequency
    for (int n = 0; n < 10; n++){
        printf("Number %d is %s with a count of %d\n", n + 1, 
           wordList[n].word,
           wordList[n].frequency);
    }

    //**************************************************************
    // DO NOT CHANGE THIS BLOCK
    //Clock output
    clock_gettime(CLOCK_REALTIME, &endTime);
    time_t sec = endTime.tv_sec - startTime.tv_sec;
    long n_sec = endTime.tv_nsec - startTime.tv_nsec;
    if (endTime.tv_nsec < startTime.tv_nsec)
        {
        --sec;
        n_sec = n_sec + 1000000000L;
        }

    printf("Total Time was %ld.%09ld seconds\n", sec, n_sec);
    //**************************************************************

    //must close the opened file
    close(fileID);

    //free the lists
    for(int i=0; i< listLength; i++){
        free(wordList[i].word);
        wordList[i].word = NULL;
    }   
    free(wordList);
    wordList = NULL;

    return 0;
}