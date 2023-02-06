/*
    Author:         Long To Lotto Tang
    Date:           11/30/2022
    File:           dec_server.c
    Description:    connect with dec_client.c to perform otp decryption
    Usage:          ./dec_server port
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/wait.h>    // waitpid()

// Error function used for reporting issues
void error(const char *msg, int exitNumber) {
  fprintf(stderr, "%s", msg);
  exit(exitNumber);
}

// Set up the address struct for the server socket
void setupAddressStruct(struct sockaddr_in* address, int portNumber){
 
  // Clear out the address struct
  memset((char*) address, '\0', sizeof(*address)); 

  // The address should be network capable
  address->sin_family = AF_INET;
  // Store the port number
  address->sin_port = htons(portNumber);
  // Allow a client at any address to connect to this server
  address->sin_addr.s_addr = INADDR_ANY;
}

void receiveContent(int socketFD, char *outputBuffer, int encryptedLength) {
  char textBuffer[1000];
  int charsRead, charsWritten;
  
  // initialize the buffer for safe operation
  memset(textBuffer, '\0', sizeof(textBuffer));
  charsRead = 0; charsWritten = 0;
  
  while (charsRead < encryptedLength) {
    charsRead += recv(socketFD, textBuffer, sizeof(textBuffer) - 1, 0);
    
    // testing - passed
    // printf("%s", textBuffer);

    if (charsRead < 0) {
      error("SERVER: ERROR reading from socket.\n", 1);
    }
    // outputBuffer should contain the entire piece of content
    strcat(outputBuffer, textBuffer);
    memset(textBuffer, '\0', 1000);
  }

  // testing
  //printf("%d\n", charsRead);
}

// perform otp encryption
void otpDecryption(char *encryptedText, char *key, char *plainText, int encryptedLength) {

  int encryptedNumber;
  int keyNumber;
  int plainNumber;

  for (int i = 0; i < encryptedLength; i++) {
    if(encryptedText[i] == ' ') {
      encryptedNumber = 26;
    } else {
      // subtracting with the number in ASCII 
      encryptedNumber = encryptedText[i] - 'A';
    }

    if(key[i] == ' ') {
      keyNumber = 26;
    } else {
      keyNumber = key[i] - 'A';
    }
    
    // ensure the number is within 0 - 26
    plainNumber = (encryptedNumber - keyNumber + 27) % 27;

    if (plainNumber == 26) {
      plainText[i] = ' ';
    } else {
      plainText[i] = 'A' + plainNumber;
    }  
    // testing - passed
    //printf("%d: %d: %c\n", i, plainNumber, plainText[i]);
    
  }
}

int main(int argc, char*argv[]) {

  if (argc != 2) {
    error("USAGE: ./dec_server port.\n", 1);
  }
  
  int connectionSocket, charsRead, charsWritten, status;
  struct sockaddr_in serverAddress, clientAddress;

  // Create the socket that will listen for connections
  int listenSocket = socket(AF_INET, SOCK_STREAM, 0);
  if (listenSocket < 0) {
    error("ERROR opening socket.\n", 1);
  }

  // Set up the address struct for the server socket
  setupAddressStruct(&serverAddress, atoi(argv[1]));

  // Associate the socket to the port
  if (bind(listenSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0){
    error("ERROR on binding.\n", 1);
  }

  // Start listening for connetions. Allow up to 5 connections to queue up
  listen(listenSocket, 5);

  socklen_t sizeOfClientInfo = sizeof(clientAddress);

  // server infinite loop
  while(1) {
    
    // Accept the connection request which creates a connection socket
    connectionSocket = accept(listenSocket, (struct sockaddr *)&clientAddress, &sizeOfClientInfo); 
    if (connectionSocket < 0){
      error("ERROR on accept.\n", 1);
    }

    pid_t pid = fork();
    // start get the message from client
    switch(pid) {
      case -1: 
        {
          error("SERVER: Fork() failed.\n", 1);
        }
      // child process
      case 0:
        {
          char buffer[1000];
          // should receive "enc_client" for verification first
          charsRead = recv(connectionSocket, buffer, sizeof(buffer) - 1, 0);
          //testing
          //printf("SERVER: received: %s\n.", buffer);
          if (charsRead < 0) {
            error("SERVER: ERROR reading from socket.\n", 1);
          }
          if (strcmp(buffer, "dec_client") != 0) {
            charsRead = send(connectionSocket, "Invalid", 7, 0);
            error("SERVER: Connection to client failed.\n", 2);
          } else {
            charsRead = send(connectionSocket, "Valid", 5, 0);
          }
          // receive the encryptedText length
          memset(buffer, '\0', sizeof(buffer));
          charsRead = 0; charsWritten = 0;
          charsRead = recv(connectionSocket, buffer, sizeof(buffer), 0);
          if (charsRead < 0) {
            error("SERVER: ERROR reading from socket.\n", 1);
          }
          // convert it to integer
          int encryptedLength = atoi(buffer);
          
          // testing - passed
          //printf("SERVER: received length: %d.\n", encryptedLength);
          
          // echo back
          charsWritten = send(connectionSocket, buffer, strlen(buffer), 0);

          // receive the key length
          memset(buffer, '\0', sizeof(buffer));
          charsRead = 0; charsWritten = 0;
          charsRead = recv(connectionSocket, buffer, sizeof(buffer), 0);
          if (charsRead < 0) {
            error("SERVER: ERROR reading from socket.\n", 1);
          }
          // convert it to integer
          int keyLength = atoi(buffer);
                    
          // echo back
          charsWritten = send(connectionSocket, buffer, strlen(buffer), 0);

          // receive the content from client
          char *encryptedBuffer = calloc(encryptedLength, sizeof(char));
          receiveContent(connectionSocket, encryptedBuffer, encryptedLength);


          char *keytextBuffer = calloc(keyLength, sizeof(char));
          receiveContent(connectionSocket, keytextBuffer, keyLength);
          
          // testing - passed
          //printf("%s", encryptedBuffer);
          //printf("%s", keytextBuffer);

          char *plainText = calloc(encryptedLength, sizeof(char));
          
          // plainText should fill with the original plainText after the function
          otpDecryption(encryptedBuffer, keytextBuffer, plainText, encryptedLength);

          // send back the result to client
          charsRead = 0; charsWritten = 0;
          while (charsWritten < encryptedLength) {
            charsWritten += send(connectionSocket, plainText, strlen(plainText), 0);
            if (charsWritten < 0) {
              error ("SERVER: ERROR writing to socket.\n", 1);
            }
          }

          // free up the dynamic memory
          free(encryptedBuffer);
          free(keytextBuffer);
          free(plainText);  
          
          exit(0);

        }
      // parent process, wait child to terminate
      default: 
        {
          while (pid > 0) {
            pid = waitpid(-1, &status, WNOHANG);
          }
        }
    }
    close(connectionSocket);
  }
  close(listenSocket);
  return 0;
}

