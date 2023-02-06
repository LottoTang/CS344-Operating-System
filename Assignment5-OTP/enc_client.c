/*
    Author:         Long To Lotto Tang
    Date:           11/30/2022
    File:           enc_client.c
    Description:    connect with enc_server.c to perform otp encryption
    Usage:          ./enc_client plaintext key port
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>  // ssize_t
#include <sys/socket.h> // send(),recv()
#include <netdb.h>      // gethostbyname()
#include <ctype.h>      // isspace(), isupper()
#include <fcntl.h>      // open()

// Error function used for reporting issues
void error(const char *msg, int exitNumber) { 
  fprintf(stderr, "%s", msg);
  exit(exitNumber); 
}

// Set up the address struct
void setupAddressStruct(struct sockaddr_in* address, int portNumber, char* hostname) {
 
  // Clear out the address struct
  memset((char*) address, '\0', sizeof(*address)); 

  // The address should be network capable
  address->sin_family = AF_INET;
  // Store the port number
  address->sin_port = htons(portNumber);

  // Get the DNS entry for this host name
  struct hostent* hostInfo = gethostbyname(hostname); 
  if (hostInfo == NULL) { 
    error("CLIENT: ERROR, no such host.\n", 1); 
  }
  // Copy the first IP address from the DNS entry to sin_addr.s_addr
  memcpy((char*) &address->sin_addr.s_addr, hostInfo->h_addr_list[0], hostInfo->h_length);
}

// return the number of characters of the file
int characterCounter (FILE* inputFD) {
    
    int character;
    int counter = 0;

    for(;;) {
        
        // check character one by one
        character = fgetc(inputFD);
        
        if (character == '\n' || character == EOF)
            break;
        
        // if cannot convert to uppercase OR not an empty character
        // bad character found
        if (!isspace(character) && (!isupper(character))) {
            error("Bad character spotted.\n", 1);
        } else {
            counter++;
        }
    }
    fclose(inputFD);
    return counter;
}

// to verify the connection between client and server
// send 1 is valid; 0 is invalid
int connectionVerification(int socketFD) {
  
  char* verifyKey = "enc_client";
  int charsRead = 0;
  int charsWritten = 0;

  charsWritten = send(socketFD, verifyKey, strlen(verifyKey), 0);
  if (charsWritten < 0) {
    error("CLIENT: ERROR writing to socket.\n", 1);
  }
  // only store the short response from the server
  char tempBuffer[20];
  memset(tempBuffer, '\0', sizeof(tempBuffer));
  charsRead = recv(socketFD, tempBuffer, sizeof(tempBuffer), 0);
  // testing - passed : received "Valid"
  //printf("CLIENT: recieved %s", tempBuffer);
  
  if (charsRead < 0) {
    error("CLIENT: ERROR reading from socket.\n", 1);
  }
  // we will define the response form server as "Valid" once connection succeeds
  if (strcmp(tempBuffer, "Valid") == 0)
    return 1;
  else
    return 0;
}

// to send the file content to server
void sendContent(int inputFD, int socketFD, int plaintextLength) {
  
  int charsRead, charsWritten;
  char buffer[1000];

  charsRead = 0; charsWritten = 0;
  while (charsWritten < plaintextLength) {
    memset(buffer, '\0', sizeof(buffer));
    // read the file's content into buffer
    charsRead = read(inputFD, buffer, sizeof(buffer) - 1);
    
    // testing - passed - charsRead: 37 (plaintext1)
    // printf("%d", charsRead);
    
    if (charsRead < 0) {
      error("CLIENT: ERROR reading from files.\n", 1);
    }
    // send the content in chunks to server
    charsWritten += send(socketFD, buffer, strlen(buffer), 0);

    // testing - passed
    // printf("%d", charsWritten);
    // printf("%s", buffer);

    if (charsWritten < 0) {
      error("CLIENT: ERROR writing to socket.\n", 1);
    }
  }
}

// to send the file length to server
void sendFileLength(int socketFD, char* buffer) {

  int charsRead, charsWritten;
  charsRead = 0; charsWritten = 0;

  // send the plaintext length
  charsWritten = send(socketFD, buffer, sizeof(buffer), 0);
  if (charsWritten < 0) {
    error("CLIENT: ERROR writing to socket.\n", 1);
  }

  // wait server to send back the length for confirmation
  memset(buffer, '\0', sizeof(&buffer));
  charsRead = recv(socketFD, buffer, sizeof(buffer), 0);
  if (charsRead < 0) {
    error("CLIENT: ERROR reading from socket.\n", 1);
  }
}

int main(int argc, char* argv[]) {
    
  if (argc != 4) {
    error("USAGE: ./enc_client plaintext key port.\n", 1);
  }

  // open the 2 external files
  FILE *plaintextFD = fopen(argv[1], "r");
  if (plaintextFD < 0) {
    error("ERROR: failed to open plaintextFD.\n", 1);
  }

  FILE *keyFD = fopen(argv[2], "r");
  if (keyFD < 0) {
    error("ERROR: failed to open keyFD.\n", 1);
  }

  // get the number of valid characters from the file
  int plaintextLength = characterCounter(plaintextFD);
  int keyLength = characterCounter(keyFD);

  // testing - passed
  //printf("Plaintext: %d\n", plaintextLength);
  //printf("KeyLength: %d\n", keyLength);
  
  if (keyLength < plaintextLength) {
    error("ERROR: The key is too short.\n", 1);
  }

  // arrive here when validated the input files
  // start setting up the server-client connection
  int socketFD, portNumber, charsWritten, charsRead;
  struct sockaddr_in serverAddress;
    
  // create a socket
  socketFD = socket(AF_INET, SOCK_STREAM, 0); 
  if (socketFD < 0){
    error("CLIENT: ERROR opening socket.\n", 1);
  }

  // Set up the server address struct
  setupAddressStruct(&serverAddress, atoi(argv[3]), "localhost");

  // Connect to server
  if (connect(socketFD, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0){
    error("CLIENT: ERROR connecting.\n", 1);
  }

  // verify connection between server
  int connection_status = connectionVerification(socketFD);
  // case for connection failed
  if (connection_status == 0) {
    error("ERROR: cannot connect to server enc_server.\n", 2);
  }

  // arrive here when connection between client and server is successful
   
  // send the length of the plaintext
  char buffer[1000];
  memset(buffer, '\0', sizeof(buffer));
  sprintf(buffer, "%d", plaintextLength);
  sendFileLength(socketFD, buffer);

  // send the length of the key
  memset(buffer, '\0', sizeof(buffer));
  sprintf(buffer, "%d", keyLength);
  sendFileLength(socketFD, buffer);

  //testing - passed : (plaintext1's length)
  //printf("CLIENT: receive the length %s", buffer);
    
  // start sending the plaintext content
  int contentPlaintextFD = open(argv[1], 'r');
  if (contentPlaintextFD < 0) {
    error ("ERROR: failed to open plaintext.\n", 1);
  }

  // send the content to server
  sendContent(contentPlaintextFD, socketFD, plaintextLength);
  
  // for plaintext4; if no sleep the server stuck
  // deliberately large number 10000
  if (plaintextLength > 10000)
    sleep(1);

  // start sending the key content
  int contentKeyFD = open(argv[2], 'r');
  if (contentKeyFD < 0) {
    error ("ERROR: failed to open key.\n", 1);
  }

  // send the content to server
  sendContent(contentKeyFD, socketFD, keyLength);
  
  // receive the encrypted text
  charsRead = 0; charsWritten = 0;
  char *encryptedText = calloc(plaintextLength, sizeof(char));

  while (charsRead < plaintextLength) {
    memset(buffer, '\0', sizeof(buffer));
    charsRead += recv(socketFD, buffer, sizeof(buffer), 0);
    if (charsRead < 0) {
      error("CLIENT: ERROR reading from socket.\n", 1);
    }
    // fill in the string piece by piece
    strcat(encryptedText, buffer);
  }
  // encryptedText should fill with the encryption password
  strcat(encryptedText, "\n");
  printf("%s", encryptedText);
  
  free(encryptedText);
  
  close(socketFD);
  return 0;
}
