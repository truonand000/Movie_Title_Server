// Movie Query Client program.
//
// Edited by: Andrew Truong
// Date: 4/17/2019

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "includes/QueryProtocol.h"
#include "QueryClient.h"

char *port_string = "1500";
unsigned short int port;
char *ip = "127.0.0.1";

#define BUFFER_SIZE 1000

// Runs a single query by connecting with the movie server, then ends the
// connection after the results are received and server sends "GOODBYE" message.
void RunQuery(char *query) {
  // Instantiate variables for the connection.
  int socketfd, bytes_sent, bytes_received, check;
  char buffer[BUFFER_SIZE];
  struct addrinfo hints, *res;

  // Set family to unspecified.
  hints.ai_family = AF_UNSPEC;

  // Make sure that hints is empty.
  memset(&hints, 0, sizeof(hints));

  // Set socket type.
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;

  // Get address info then make a socket.
  if ((check = getaddrinfo(ip, port_string, &hints, &res)) != 0) {
    perror("getaddrinfo");
    return;
  }

  socketfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);

  // Connect to the server.
  if (connect(socketfd, res->ai_addr, res->ai_addrlen) == -1) {
    freeaddrinfo(res);
    close(socketfd);
    perror("connecting");
    return;
  }

  // Free address info, no longer needed.
  freeaddrinfo(res);

  // Message for user showing successful connection.
  printf("Connected to Movie Query Server.\n\n");

  // Recieve and confirm acknowledgement info.
  if ((bytes_received = recv(socketfd, buffer, BUFFER_SIZE, 0)) == -1) {
    close(socketfd);
    perror("Client Recieve");
    return;
  }

  buffer[bytes_received] =  '\0';

  if (CheckAck(buffer) == 0) {

    // Begin query protocol.
    buffer[0] = 0;
    if ((bytes_sent = send(socketfd, query, strlen(query), 0)) == -1) {
      close(socketfd);
      perror("Client send");
      return;
    }
    if ((bytes_received = recv(socketfd, buffer, BUFFER_SIZE, 0)) == -1) {
      close(socketfd);
      perror("Client Recieve");
      return;
    }
    buffer[bytes_received] =  '\0';
    printf("Number of Results: %s\n", buffer);
    int number_of_results = atoi(buffer);
    if (number_of_results == 0) {
      close(socketfd);
      printf("There are no results for this query\n");
      return;
    }

    // Loop that receives and prints results. Terminates upon 'GOODBYE' message.
    while (1) {
      SendAck(socketfd);
      if ((bytes_received = recv(socketfd, buffer, BUFFER_SIZE, 0)) == -1) {
	close(socketfd);
	perror("Client Recieve");
	return;
      }
      buffer[bytes_received] = '\0';
      if (strcmp(buffer, "GOODBYE") == 0) {
	break;
      }
      printf("%s\n", buffer);
    }
  }
  // End connection. Close socket.
  close(socketfd);
}

// Loops and asks the client for a term to search for, then calls on RunQuery.
void RunPrompt() {
  char input[BUFFER_SIZE];

  while (1) {
    printf("Enter a term to search for, or q to quit: ");
    scanf("%s", input);

    printf("input was: %s\n", input);

    if (strlen(input) == 1) {
      if (input[0] == 'q') {
        printf("Thanks for playing! \n");
        return;
      }
    }
    printf("\n\n");
    RunQuery(input);
  }
}

int main(int argc, char **argv) {
  // Check/get arguments
  if (argc != 3) {
    printf("The number of arguments is invalid.\n");
    printf("Please run the program again using the following format:\n");
    printf("'./queryclient <IP address> <port number>\n");
    printf("EXITING NOW...\n");
    return 0;
  } else {
    ip = argv[1];
    port_string = argv[2];
  }

  RunPrompt();

  return 0;
}
