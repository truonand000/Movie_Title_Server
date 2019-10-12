// Program representing a server that is able to use fork() to
// serve multiple clients.
//
// Edited by: Andrew Truong
// Date: 4/22/2019

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>


#include "QueryProtocol.h"
#include "MovieSet.h"
#include "MovieIndex.h"
#include "DocIdMap.h"
#include "htll/Hashtable.h"
#include "QueryProcessor.h"
#include "FileParser.h"
#include "FileCrawler.h"

#define BUFFER_SIZE 1000

int Cleanup();

DocIdMap docs;
Index docIndex;

// Global variables to be shared across methods.
// Socketfds are global for easy cleanup.
int socketfd;
int client_socketfd;
int bytes_received;
#define SEARCH_RESULT_LENGTH 1500
#define BACKLOG_SIZE 10

struct sockaddr_storage client_addr_storage;
socklen_t addr_size;

char buffer[BUFFER_SIZE];
char movieSearchResult[SEARCH_RESULT_LENGTH];

void sigchld_handler(int s) {
  write(0, "Handling zombies...\n", 20);
  // waitpid() might overwrite errno, so we save and restore it:
  int saved_errno = errno;

  while (waitpid(-1, NULL, WNOHANG) > 0);

  errno = saved_errno;
}


void sigint_handler(int sig) {
  write(0, "Ahhh! SIGINT!\n", 14);
  Cleanup();
  exit(0);
}

// Function used to handle a single connection and query from the client.
// Sends a Goodbye message and closes the connection after this query is finished.
void runQuery(int client_socketfd, char *buffer) {
  int result, bytes_received;
  SearchResultIter results = FindMovies(docIndex, buffer);

  if (results == NULL) {
    // If no results, sends Goodbye message and ends the connection.
    send(client_socketfd, "0", strlen("0"), 0);
    printf("No results for this term. Please try another.\n");
    printf("Closing Client Connection...\n");
    SendGoodbye(client_socketfd);
    return;
  } else {
    sprintf(movieSearchResult, "%d", results->numResults);
    printf("Number of Results: %s\n", movieSearchResult);
    send(client_socketfd, movieSearchResult, strlen(movieSearchResult), 0);

    SearchResult sr = (SearchResult)malloc(sizeof(*sr));
    if (sr == NULL) {
      printf("Couldn't malloc SearchResult\n");
      return;
    }

    // Uses SearchResultIter to iterate through the movie index and sends results
    // to the client.
    bytes_received = recv(client_socketfd, buffer, BUFFER_SIZE, 0);
    buffer[bytes_received] = '\0';
    if (CheckAck(buffer) != 0) {
      return;
    }
    SearchResultGet(results, sr);
    CopyRowFromFile(sr, docs, movieSearchResult);
    send(client_socketfd, movieSearchResult, strlen(movieSearchResult), 0);

    while (SearchResultIterHasMore(results) != 0) {
      // sleep(1); // Sleep used for testing multiprocessessing.
      result =  SearchResultNext(results);
      if (result < 0) {
        printf("error retrieving result\n");
        break;
      }
      bytes_received = recv(client_socketfd, buffer, BUFFER_SIZE, 0);
      buffer[bytes_received] = '\0';
      if (CheckAck(buffer) != 0) {
        return;
      }
      SearchResultGet(results, sr);
      CopyRowFromFile(sr, docs, movieSearchResult);
      send(client_socketfd, movieSearchResult, strlen(movieSearchResult), 0);
    }

    // Sends Goodbye message and ends the connection.
    printf("Closing Client Connection...\n");
    SendGoodbye(client_socketfd);
    free(sr);
    DestroySearchResultIter(results);
  }
}

// Handles multiple connections by forking everytime a connection is made.
// Single parent process that loops through and starts connections while child
// processes finish the query. Child processes then exit upon query completion.
int HandleConnections(int sock_fd) {
  addr_size = sizeof(client_addr_storage);
  while (1) {
    printf("Waiting for client connection...\n");
    client_socketfd = accept(sock_fd, (struct sockaddr *)&client_addr_storage, &addr_size);
    printf("Client connected. Forking Process...\n");
    if (!fork()) {
      close(socketfd);
      SendAck(client_socketfd);
      bytes_received = recv(client_socketfd, buffer, BUFFER_SIZE, 0);
      buffer[bytes_received] = '\0';
      printf("Query Received: %s \n", buffer);
      runQuery(client_socketfd, buffer);
      close(client_socketfd);
      exit(0);
    }
  }
}

// Sets up clean up structures and builds the movie index.
void Setup(char *dir) {
  struct sigaction sa;

  sa.sa_handler = sigchld_handler;  // reap all dead processes
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_RESTART;
  if (sigaction(SIGCHLD, &sa, NULL) == -1) {
    perror("sigaction");
    exit(1);
  }

  struct sigaction kill;

  kill.sa_handler = sigint_handler;
  kill.sa_flags = 0;  // or SA_RESTART
  sigemptyset(&kill.sa_mask);

  if (sigaction(SIGINT, &kill, NULL) == -1) {
    perror("sigaction");
    exit(1);
  }

  printf("Crawling directory tree starting at: %s\n", dir);
  // Create a DocIdMap
  docs = CreateDocIdMap();
  CrawlFilesToMap(dir, docs);
  printf("Crawled %d files.\n", NumElemsInHashtable(docs));

  // Create the index
  docIndex = CreateIndex();

  // Index the files
  printf("Parsing and indexing files...\n");
  ParseTheFiles(docs, docIndex);
  printf("%d entries in the index.\n", NumElemsInHashtable(docIndex->ht));
}

// Cleans up program after it exits.
int Cleanup() {
  DestroyOffsetIndex(docIndex);
  DestroyDocIdMap(docs);
  close(socketfd);
  return 0;
}

int main(int argc, char **argv) {
  // Get args
  char *dir_to_crawl, *port_number;
  if (argc != 3) {
    printf("Incorrect number of arguments.\n");
    printf("Please use the following format when running the program: \n");
    printf("./multiserver <directory_to_index> <port_number>\n");
    printf("NOW EXITING...\n");
    return 0;
  } else {
    // Set up structs for socket opening, port binding.
    struct addrinfo hints, *server_info;
    int check;
    int yes = 1;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;


    dir_to_crawl = argv[1];
    port_number = argv[2];
    // Setup graceful exit
    struct sigaction kill;

    kill.sa_handler = sigint_handler;
    kill.sa_flags = 0;  // or SA_RESTART
    sigemptyset(&kill.sa_mask);

    if (sigaction(SIGINT, &kill, NULL) == -1) {
      perror("sigaction");
      exit(1);
    }

    Setup(dir_to_crawl);

    // Step 1: get address/port info to open
    if ((check = getaddrinfo(NULL, port_number, &hints, &server_info)) != 0) {
      fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(check));
      exit(1);
    }

    // Open socket.
    if ((socketfd = socket(server_info->ai_family, server_info->ai_socktype,
                           server_info->ai_protocol)) == -1) {
      perror("Server: socket\n");
      exit(1);
    }

    // Clears addresses to avoid "address already in use" error.
    if (setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
      perror("setsockopt");
      exit(1);
    }

    // Binding socket to port.
    if (bind(socketfd, server_info->ai_addr, server_info->ai_addrlen) == -1) {
      perror("Server bind");
      exit(1);
    }

    // Frees address (no longer using).
    freeaddrinfo(server_info);

    // Listen for connections.
    listen(socketfd, BACKLOG_SIZE);

    // Handle connections.
    HandleConnections(socketfd);

  }
  Cleanup();
  return 0;
}
