/**
 * OSU CS344 Assignment - MTP
 * 
 * Author: Long To Lotto Tang
 * Date: 11/11/2022
 *
 * Description: 
 *
 *  - create a program that creates 4 threads to process input from standard input (keyboard / file)
 *  - output to stdout only lines with 80 characters (with a line separator after each line)
 *  - pairs of plus signs must be replaced (e.g. abc+++def => abc^+def)
 *  - 4 threads must communicate with each other using Producer-Consumer approach
 *
 *  Code reference to course material - 6_5_prod_cons_pipeline.c
 *  https://replit.com/@cs344/65prodconspipelinec
 **/
#define _POSIX_C_SOURCE 200809L

// treat the program in unbounded buffer approach
#define MAXINPUTCHAR 1000
#define MAXLINE 50
#define MAXCHARPERLINE 80

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

//Buffer 1, shared resource between input and thread2
// max row: 50; max char in a row: 1000
char buffer_1[MAXLINE][MAXINPUTCHAR];

//number of items in the buffer
int count_1 = 0;

//index where the input thread will put the next item
int prod_idx_1 = 0;

//index where the thread2 will pick up the item
int con_idx_1 = 0;

//initialize mutex for buffer1
pthread_mutex_t mutex_1 = PTHREAD_MUTEX_INITIALIZER;

//initialize the condition variable for buffer1
pthread_cond_t full_1 = PTHREAD_COND_INITIALIZER;

//Buffer2 related variables
char buffer_2[MAXLINE][MAXINPUTCHAR];
int count_2 = 0;
int prod_idx_2 = 0;
int con_idx_2 = 0;
pthread_mutex_t mutex_2 = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t full_2 = PTHREAD_COND_INITIALIZER;

//Buffer3 related variables
char buffer_3[MAXLINE][MAXINPUTCHAR];
int count_3 = 0;
int prod_idx_3 = 0;
int con_idx_3 =0;
pthread_mutex_t mutex_3 = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t full_3 = PTHREAD_COND_INITIALIZER;


// get input from user
// simple checking for empty entry; exit directly
char* getUserInput() {
  char *buffer = NULL;
  size_t bufferSize = MAXINPUTCHAR;
  getline(&buffer, &bufferSize, stdin);

  if (strcmp(buffer, "\n") == 0) {
    printf("No input spotted\n");
    fflush(stdout);
    exit(1);
  }
  return buffer;
}

void put_buff_1(char *userInput) {
  // lock the mutex before putting the item in the buffer
  pthread_mutex_lock(&mutex_1);
  // put the userInput in buffer
  strcpy(buffer_1[prod_idx_1], userInput);
  //increment the index for next entry and counter
  prod_idx_1 = prod_idx_1 + 1;
  count_1++;
  //signal to the consumer that the buffer is no longer empty (trigger the consumer)
  pthread_cond_signal(&full_1);
  //unlock the mutex for other thread
  pthread_mutex_unlock(&mutex_1);
}

// once STOP is spotted, no getting user input is needed
void *get_input(void *args) {
  // once STOP is found, the loop will break
  for (;;) {
    char *userInput = getUserInput();
    //put the user input to buff_1
    put_buff_1(userInput);
    // check if the getline item is exactly STOP (e.g. stop, Stop, STOP!, ISTOP do not count as valid)
    if(strcmp(userInput, "STOP\n") == 0)
      break;
  }
  return NULL;
}

// get the next userInput from buffer1
char* get_buff_1() {
  //lock the mutex before checking if the buffer has data
  pthread_mutex_lock(&mutex_1);
  while (count_1 == 0)
    pthread_cond_wait(&full_1, &mutex_1);
  char *userInput = buffer_1[con_idx_1];
// update the index for next entry and counter in consumer
  con_idx_1 = con_idx_1 + 1;
  // consumer picked up the content, counter should minus one
  count_1--;
  pthread_mutex_unlock(&mutex_1);
  return userInput;
}

// put userInput into buffer
void put_buff_2(char* userInput) {

  //lock the mutex for thread2 before any processing
  pthread_mutex_lock(&mutex_2);
  // put the item in buffer
  strcpy(buffer_2[prod_idx_2], userInput);
  
  //increment the index for next entry and counter
  prod_idx_2 = prod_idx_2 + 1;
  count_2++;

  // signal the consumer (thread2) that it has something in the buffer now
  pthread_cond_signal(&full_2);

  //unlock for consumer to access the shared resources
  pthread_mutex_unlock(&mutex_2);
}


// scan every \n from the buffer_1 (the userInput), process it and replace all \n to  ' '
void* lineSeparatorProcessing(void* args) {
  char* processedInput;
  for (;;) {
    processedInput = get_buff_1();
    // scan through the whole char array here
    for(int i = 0; i < strlen(processedInput); i++) {
      //replace the \n to ' '
      if(processedInput[i] == '\n') {
        processedInput[i] = ' ';
      }
    }
    put_buff_2(processedInput);
    //check if STOP is spotted to determinate the input (originally "STOP\n"
    if(strcmp(processedInput, "STOP ") == 0)
      break;
  }

  return NULL;
}

// get the processedInput (without \n) from buffer2
char *get_buff_2() {
  //lock the mutex of thread2 before any processing
  pthread_mutex_lock(&mutex_2);
  // as there is no input, wait for the input 
  while (count_2 == 0) 
    pthread_cond_wait(&full_2, &mutex_2);
  char *processedInput = buffer_2[con_idx_2];
  
  // update number of counter
  con_idx_2 = con_idx_2+1;
  count_2--;

  //unlock the mutex for other processing
  pthread_mutex_unlock(&mutex_2);
  return processedInput;
  
}

// put the processed input (++ replaced with ^) to buffer
void put_buff_3(char* userInputPlus) {
  //lock the shared variable before processing
  pthread_mutex_lock(&mutex_3);

  strcpy(buffer_3[prod_idx_3], userInputPlus);

  prod_idx_3 = prod_idx_3 + 1;
  count_3++;
  // signal to the consumer thread that there is input
  pthread_cond_signal(&full_3);
  // unlock the mutex for other process
  pthread_mutex_unlock(&mutex_3);

}

//thread 3: handle ++ sign to ^
void *handlePlusSign() {
  char *userInputPlus;
  int length = -1;
  for (;;) {
    userInputPlus = get_buff_2();
    // scan through the char array, check any "++" and replaced with '^'
    for (int i = 0; i < strlen(userInputPlus)-1; i++) {
      if ((userInputPlus[i] == '+') && (userInputPlus[i+1] == '+')) {
        userInputPlus[i] = '^';

        // need to push the char at the back to fill up the space of userInputPlus[i+1]
        // e.g. ABC++DEFG --> ABC^DEFG\0
        char temp;
        for (int j = i + 1; j < strlen(userInputPlus)-1; j++) {
          temp = userInputPlus[j+1];
          userInputPlus[j] = temp;
        }
        // handle the last character filled with \0
        userInputPlus[strlen(userInputPlus)-1] = '\0';
      }
    }
    put_buff_3(userInputPlus);
    //
    if(strcmp(userInputPlus, "STOP ") == 0)
      break;
  }

  return NULL;
}

// get the procsssed input (++ replaced with ^) from buffer
char *get_buff_3() {
  //lock the mutex of thread3 before any processing
  pthread_mutex_lock(&mutex_3);
  //if there is no available input, wait it
  while(count_3 == 0)
    pthread_cond_wait(&full_3, &mutex_3);
  char *userInputPlus = buffer_3[con_idx_3];

  //update number of counter
  con_idx_3 = con_idx_3 + 1;
  count_3--;

  //unlock the mutex
  pthread_mutex_unlock(&mutex_3);
  return userInputPlus;
}

// write output to stdout
// limit 80 characters a line for the output
// get the finalized processed input from buffer3
void *stdoutOutput (void* arg) {
  char *finalizedInput;
  char outputArray[MAXINPUTCHAR];
  int counter = 0;

  for(;;) {
    finalizedInput = get_buff_3();
    for(int i = 0; i < strlen(finalizedInput); i++) {
      outputArray[counter] = finalizedInput[i];
      counter++;
      // only when 80 characters will output to stdout
      if (counter == MAXCHARPERLINE) {
        for (int k = 0; k < MAXCHARPERLINE; k++) 
          printf("%c", outputArray[k]);
        printf("\n");
        counter = 0;
      }
    }
    if(strcmp(finalizedInput, "STOP ") == 0)
      break;
  }

  return NULL;
}

int main() {
  
  srand(time(0));
  pthread_t inputThread, lineSeparatorThread, plusSignThread, outputThread;

  // create the threads
  pthread_create(&inputThread, NULL, get_input, NULL);
  pthread_create(&lineSeparatorThread, NULL, lineSeparatorProcessing, NULL);
  
  pthread_create(&plusSignThread, NULL, handlePlusSign, NULL);
  pthread_create(&outputThread, NULL, stdoutOutput, NULL);
  

  // wait for threads to terminate
  pthread_join(inputThread, NULL);
  pthread_join(lineSeparatorThread, NULL);
  pthread_join(plusSignThread, NULL);
  pthread_join(outputThread, NULL);

  //testing
  /*
  for (int i = 0; i < prod_idx_1; i++)
    printf("%s", buffer_3[i]);*/

  return EXIT_SUCCESS;
}
        

