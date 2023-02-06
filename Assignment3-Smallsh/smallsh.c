/**
 *
 *  Author: Long To Lotto Tang
 *  Project: OSU CS344 - SMALLSH
 *  Date: 4 Nov 2022
 *
 *  Description: Mockup of Unix shell within limited commands: cd, exit, status
 *               Signal handling with SIGINT (Control C) & SIGTSTP (Control Z)
 *               Create child process with fork(), exec(), waitpid()
 *  
 *  Remarks: 
 *  1) General syntax of a command line:
 *  - command [arg1 arg2...] [< input file] [> output file] [&}
 *
 *  2) $$ - Expansion of variable to process ID
 *
 *  3) possible bug: grading script - kill -SIGTSTP $$ ; will prompt background process terminated message but sample does not
 *
 **/ 

#define _POSIX_C_SOURCE 200809L

#include <fcntl.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdbool.h>

//global constants

#define MAX_LENGTH 2048
#define MAX_ARG 512

// global variables
bool inForegroundModeOnly = false;
bool sigtstpTriggered = false;

// custom signal handler function for SIGTSTP
void sigtstp_handler(int signo) {

  // switch the current mode to foreground-mode only
  if (inForegroundModeOnly == false) {

    char *prompt = "\nEntering foreground-only mode (& is now ignored)\n";
    write(STDOUT_FILENO, prompt, 50);
    inForegroundModeOnly = true;

  }
  // if now is in foreground-only mode and trigger TSTP Signal again, switch back to normal mode
  else {

    char *prompt = "\nExiting foreground-only mode\n";
    write(STDOUT_FILENO, prompt, 30);
    inForegroundModeOnly = false;
  }

  fflush(stdout);
  sigtstpTriggered = true;
}

// get user input, exclude empty space and store to userArg
void userCommand(char **userArg, int *argCounter) {
  char inputBuffer[MAX_LENGTH] = {'\0'};

  // ask for command from user
  do {
    printf(": ");
    fgets(inputBuffer, MAX_LENGTH, stdin);
    //to remove the newline character \n as part of the command
    inputBuffer[strcspn(inputBuffer, "\n")] = '\0';

  // check for comment / empty line / argument over defined limited
  } while (inputBuffer[0] == '#' || strlen(inputBuffer) < 1 || strlen(inputBuffer) > MAX_LENGTH);

  // check if the input contains $$
  for (int i = 0; i < strlen(inputBuffer)-1; i++) {
      if (inputBuffer[i] == '$' && inputBuffer[i+1] == '$') {
        // temporarily copy the original inputBuffer to dupInputBuffer
        char *dupInputBuffer = strdup(inputBuffer);

        // replace the $$ with process ID
        dupInputBuffer[i] = '%';
        dupInputBuffer[i+1] = 'd';
        pid_t currentPID = getpid();
        sprintf(inputBuffer, dupInputBuffer, currentPID);

        free(dupInputBuffer);
      }
  }

  // Assign inputBuffer back to userArg
  char delimitor[] = " ";
  
  // splitting the user command by " "
  // token should contain the text of string in before any ' ' character
  char *token = strtok(inputBuffer, delimitor);

  // loop through the inputBuffer, assign back to reference parameter
  while (token != NULL) {
    userArg[*argCounter] = strdup(token);
    (*argCounter)++;
    token = strtok(NULL, delimitor);
  }
}

// print out the exit status of the process;
void exitStatus(int codeStatus) {

    // process to terminate normally
    if(WIFEXITED(codeStatus)) 
      printf("exit value %d\n", WEXITSTATUS(codeStatus));
    
    //  terminated by signal
    else if(WIFSIGNALED(codeStatus))
      printf("terminated by signal %d\n", WTERMSIG(codeStatus));
  
    fflush(stdout);
}

//check if open() failed of not
void openResult(int result, char* fileName) {
  if(result == -1) {

    // formulate formatted text as the same as grading script
    char errorMsg[MAX_LENGTH];
    snprintf(errorMsg, sizeof(errorMsg), "smallsh: %s", fileName);
    perror(errorMsg);
    exit(1);
  }
}

//check if dup2() failed or not
void dupResult(int result) {
  if(result == -1) {
    perror("dup2");
    exit(1);
  }
}


void shell(char **userArg, int *argCounter, bool *inBackgroundMode, struct sigaction SIGINT_action) {

  static int codeStatus;
  pid_t pid;
 
  // check if '&' is entered to indicate background process from user
  if (strcmp(userArg[(*argCounter)-1], "&") == 0) {
    // treat & not as an argument
    userArg[(*argCounter)-1] = NULL;
    // trigger background mode
    *inBackgroundMode = true;
  }

  // handle the 3 built-in commands 
  
  // command - cd
  if (strcmp(userArg[0], "cd") == 0) {
    
    // if there is specific path/file entered (cd + argu1 + argu2....)
    if(*argCounter > 1) {
      if(chdir(userArg[1]) == -1) {
        printf("Directory not found\n");
        fflush(stdout);
      }
    }
    else 
      // no argument passed, return to HOME directory
      chdir(getenv("HOME"));
  }

  // command - exit
  else if(strcmp(userArg[0], "exit") == 0) {

    // clear the output buffer before termination
    // free up the memory used when exit entered
    for (int i = 0; i < *argCounter; i++) {
      free(userArg[i]);
      userArg[i] = NULL;
    }
    fflush(stdout); 
    exit(0);
  }

  else if(strcmp (userArg[0], "status") == 0) {
    exitStatus(codeStatus);  
  }
  
  // command other than "cd, exit, status" - fork with exec that command
  else {
  
    pid = fork();
    switch(pid) {
    
      //case -1: fork failed
      case -1: 
        perror("fork() failed\n");
        exit(1);
        break;

      //case 0: child process
      case 0:
        
        //set default to SIGINT
        if(*inBackgroundMode == false || inForegroundModeOnly) {
          SIGINT_action.sa_handler = SIG_DFL;
          sigaction(SIGINT, &SIGINT_action, NULL);
        }

        int counter = 1;
        int fileDescriptor;
        bool redirection = false;

        // check for redirection
        while (counter < (*argCounter) - 1) {

          if((strcmp(userArg[counter], "<") == 0) || (strcmp(userArg[counter], ">") == 0)) {

            redirection = true;
            if(*inBackgroundMode == true) {

              //default for background process; 
              fileDescriptor = open("dev/null", O_RDONLY);
              //for stdin
              int result_1 = dup2(fileDescriptor, STDIN_FILENO);
              dupResult(result_1);
              int result_2 = dup2(fileDescriptor, STDOUT_FILENO);
              dupResult(result_2);
            }

          // not in background process
          else {

              char *fileName = userArg[counter+1];

              if(strcmp(userArg[counter], "<") == 0) {
                fileDescriptor = open(fileName, O_RDONLY);
                openResult(fileDescriptor, fileName);
                int result_1 = dup2(fileDescriptor, STDIN_FILENO);
                dupResult(result_1);            
              }

              // case for ">"
              else {
                fileDescriptor = open(fileName, O_CREAT | O_RDWR | O_TRUNC, 0644);
                openResult(fileDescriptor, fileName);
                int result_1 = dup2(fileDescriptor, STDOUT_FILENO);
                dupResult(result_1);
              }
              close(fileDescriptor);
            }
          }
          // advance to next argument check
          counter++;
        }

        //testing
        //for (int i = 0; i < *argCounter; i++)
          //printf("%d : %s", i, userArg[i]);

        // if there is file redirection, remove all arugments except the 1st (command) for calling execvp
        if(redirection == true) {
          for (int i = 1; i < (*argCounter); i++) {
            userArg[i] = NULL;
          }
        }

        // execute the command
        if(execvp(userArg[0], userArg)) {
            // if no such command
            fprintf(stderr, "%s: command not found\n", userArg[0]);
            fflush(stdout);
            exit(1);
        }

        // end of case 0
        break;
    
      // parent process
      default:
     
        // for background process execution
        if(*inBackgroundMode == true && inForegroundModeOnly == false) {
          printf("background pid is %d\n", pid);
          fflush(stdout);
        }
     
        else {
          // parent is waiting its child to terminate
          waitpid(pid, &codeStatus, 0);
        }

          // if any signal termination for the waiting child
          if(!sigtstpTriggered || inForegroundModeOnly)
            if(WIFSIGNALED(codeStatus) && WTERMSIG(codeStatus) != 0) {
              printf("terminated by signal %d\n", WTERMSIG(codeStatus));
              fflush(stdout);
            }

          // check any background child process     
          while ((pid = waitpid(-1, &codeStatus, WNOHANG)) > 0) {
            
            // do not use exitStatus() to suit thr format text output
            // if child process exits normally
            if(WIFEXITED(codeStatus)) 
              printf("background pid %d is done: exit value %d\n", pid, WEXITSTATUS(codeStatus));

            // if child terminates by signal
            else 
              printf("background pid %d is done: terminated by signal %d\n", pid, WTERMSIG(codeStatus));

            fflush(stdout);
          } 
        
      break;
    } 
  }
}


int main() {
  
  char *userArg[MAX_ARG] = {NULL};
  int argCounter = 0;
  bool inBackgroundMode = false;
  
  // to handle SIGINT, SIGTSTP
  struct sigaction SIGINT_action = {0}, SIGTSTP_action = {0};

  //set SIGINT to be ignored
  SIGINT_action.sa_handler = SIG_IGN;
  
  // block all catchable signals while SIG_IGN is reunning
  sigfillset(&SIGINT_action.sa_mask);

  // no flags set
  SIGINT_action.sa_flags = 0;

  // install the signal handler for SIGINT
  sigaction(SIGINT, &SIGINT_action, NULL);

  //set SIGTSTP to be handled bu sigtstp_handler
  SIGTSTP_action.sa_handler = sigtstp_handler;
  sigfillset(&SIGTSTP_action.sa_mask);
  SIGTSTP_action.sa_flags = 0;
  sigaction(SIGTSTP, &SIGTSTP_action, NULL); 

  for(;;) {
    
    fflush(stdout);

    userCommand(userArg, &argCounter);

    shell(userArg, &argCounter, &inBackgroundMode, SIGINT_action);
    
   
    // reset variables for next command
    for (int i = 0; i < argCounter; i++) {
      free(userArg[i]);
      userArg[i] = NULL;
    }

    argCounter = 0;
    inBackgroundMode = false;
    sigtstpTriggered = false;
  }

  return 0;
}

