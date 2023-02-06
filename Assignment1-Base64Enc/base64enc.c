// Author: Long To Lotto Tang
// OSU CS344 Base64 - Revision
//
// Description: Perform base64 encoding from stdin (keyboard or file)
// - if no file specified, get input from keyboard
// output for every 76 characters
//
// Source: from course material - https://www.youtube.com/watch?v=udZCDoNkbOo&t=4227s&ab_channel=RyanGambord
//         from Ed Discussion - https://edstem.org/us/courses/29177/discussion/1810373
//

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <err.h>
#include <stdint.h> // *should* typedef uint8_t
// Check that uint8_t type exists
#ifndef UINT8_MAX
#error "No support for uint8_t"
#endif

#define MAXCHAR 76
#define arraylen(x) (sizeof x / sizeof *x)

static char const alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                               "abcdefghijklmnopqrstuvwxyz"
                               "0123456789+/=";

int main(int argc, char* argv[]) {
  
  //========================================================
  // get stdin from user or file
  FILE *input_fp = stdin;
  // no argument passed
  if (argc == 1) {
    // do nothing
  } else if (argc == 2) {
    // "-" as reading from user input
    if (strcmp("-", argv[1]) == 0) {
      // do nothing again
    } else {
      input_fp = fopen(argv[1], "r");
      // check if it is opened successfully
      if (input_fp == NULL) {
        err(errno, "fopen()");
      }
    }
  // case for having 3 or more arguments
  } else {
    err(errno = EINVAL, "Usage: %s [FILE]", argv[0]);
  }
  
  //========================================================

  //========================================================
  // bitwise conversion
  size_t counter = 0;
  
  for(;;) {
    uint8_t in[3] = {0};
    // return a total number of elements successfully read
    size_t n = fread(in, sizeof *in, sizeof in / sizeof *in, input_fp);
    
    // case for fread returns short item count
    if (n < arraylen(in) && ferror(input_fp))
      err(errno, "fread()");
    
    // case for eof without any new character in buffer
    if (n == 0 && feof(input_fp))
      break;

    unsigned char out[4] = {0};
    out[0] = in[0] >> 2;
    out[1] = (in[0] << 4 | in[1] >> 4) & 0x3F;
    out[2] = (in[1] << 2 | in[2] >> 6) & 0x3F;
    out[3] = in[2] & 0x3F;
  
    // add padding
    if (n < 3) {
      out[3] = 64;
      out[2] &= 0x3C;
    }

    if (n < 2) {
      out[2] = 64;
      out[1] &= 0x30;
    }

    char output[arraylen(out)];
    // map the base64 encoding to the output array
    for (size_t i = 0; i < arraylen(out); i++)
      output[i] = alphabet[out[i]];

    // write the output bytes to stdout
    fwrite(output, sizeof* output, arraylen(output), stdout);

    // each iteration will have 4 characters
    counter = counter + 4;

    // check if each line contains 76 characters, if true, open a new line
    if (counter == MAXCHAR) {
      if(putchar('\n') == EOF) {
        err(errno, "putchar()");
      }
      counter = 0;
    }
    
    // if n is a short count, implying there is no more to input
    if (n < arraylen(in) && feof(input_fp))
      break; 
  }

  // always ends at a new line
  if (counter != 0) {
    if (putchar('\n') == EOF)
     err(errno, "putchar()");
  }

  // clear the buffer before termination
  if (fflush(stdout) == EOF)
    err(errno, "fflush()");

  // check if fclose() is successfull
  if (input_fp != stdin && fclose(input_fp) == EOF)
    err(errno, "fclose()");

  return EXIT_SUCCESS;
}
