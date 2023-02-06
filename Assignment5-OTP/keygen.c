/*
    Author:         Long To Lotto Tang
    Date:           11/30/2022
    File:           keygen.c
    Description:    generate [inputLength] number of characters
    Usage:          ./keygen inputLength
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

// Error function used for reporting issues
void error(const char *msg, int exitNumber) { 
  fprintf(stderr, "%s", msg); 
  exit(exitNumber); 
}

int main(int argc, char* argv[]) {
  if (argc != 2) {
    error("USAGE: ./keygen inputLength", 0);
  }

  // check the input is valid number
  int length = atoi(argv[1]);
  if (length < 0) {
    error("ERROR: Please enter an integer greater than or equal to 0.\n", 0);
  }

  const char *charArray = "ABCDEFGHIJKLMNOPQRSTUVWXYZ ";

  // to reserve the length + null terminator
  char outputArray[length + 1];

  // initialize the random generator
  srand(time(0));

  for (int i = 0; i < length; i++) {
    // to ensure index 0 - 26 is generated
    outputArray[i] = charArray[rand() % 27];
  }

  // put the null terminator
  outputArray[length] = '\0';

  // display result
  printf("%s\n", outputArray);
  return 0;
}
