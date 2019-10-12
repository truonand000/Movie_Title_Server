// Program that builds a movie server that is able to connect and serve one
// client connection at a time.
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
#include <signal.h>


#include "QueryProtocol.h"
#include "MovieSet.h"
#include "MovieIndex.h"
#include "DocIdMap.h"
#include "htll/Hashtable.h"
#include "QueryProcessor.h"
#include "FileParser.h"
#include "FileCrawler.h"

DocIdMap docs;
Index docIndex;

// Global socketfd for easy cleanup.
int socketfd;

#define BUFFER_SIZE 1000
#define SEARCH_RESULT_LENGTH 1500
#define BACKLOG_SIZE 10
char movieSearchResult[SEARCH_RESULT_LENGTH];

int Cleanup();

void sigint_handler(int sig) {
  write(0, "Exit signal sent. Cleaning up...\n", 34);
  Cleanup();
  exit(0);
}


void Setup(char *dir) {
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

int Cleanup() {
  DestroyOffsetIndex(docIndex);
  DestroyDocIdMap(docs);
  close(socketfd);
  return 0;
}

// Takes client's query search and iterates through movie index.
// Uses the client-server connection to send the results through to the client.
void runQuery(int client_socketfd, char *buffer) {
  int result, bytes_received;
  SearchResultIter results = FindMovies(docIndex, buffer);

  // Use search result iter to iterate over index and get results.
  if (results == NULL) {
    // If no results, close connection.
    send(client_socketfd, "0", strlen("0"), 0);
    printf("No results for this term. Please try another.\n");
    printf("Closing Client Connection...\n");
    SendGoodbye(client_socketfd);
    return;
  } else {

    // Send back number of results and start sending client the results.
    sprintf(movieSearchResult, "%d", results->numResults);
    printf("Number of Results: %s\n", movieSearchResult);
    send(client_socketfd, movieSearchResult, strlen(movieSearchResult), 0);

    SearchResult sr = (SearchResult)malloc(sizeof(*sr));
    if (sr == NULL) {
      printf("Couldn't malloc SearchResult\n");
      return;
    }

    bytes_received = recv(client_socketfd, buffer, BUFFER_SIZE, 0);
    buffer[bytes_received] = '\0';
    if (CheckAck(buffer) != 0) {
      return;
    }
    SearchResultGet(results, sr);
    CopyRowFromFile(sr, docs, movieSearchResult);
    send(client_socketfd, movieSearchResult, strlen(movieSearchResult), 0);

    while (SearchResultIterHasMore(results) != 0) {
      //      sleep(5);  // Used to test server with multiple clients. (Compared to multiserver)
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

    printf("Closing Client Connection...\n");
    SendGoodbye(client_socketfd);
    free(sr);
    DestroySearchResultIter(results);
  }
}


int main(int argc, char **argv) {
  // Get args
  char *dir_to_crawl, *port_number;
  if (argc != 3) {
    printf("Incorrect number of arguments.\n");
    printf("Please use the following format when running the program: \n");
    printf("./queryserver <directory_to_index> <port_number>\n");
    printf("NOW EXITING...\n");
    return 0;
  } else {

    // Set up structures for the server socket/binding/connection portion.
    struct addrinfo hints, *server_info;
    struct sockaddr_storage client_addr_storage;
    socklen_t addr_size;
    int client_socketfd, check, bytes_received;
    int yes = 1;
    char buffer[BUFFER_SIZE];

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

    // Step 2: Open the socket for the server side.
    if ((socketfd = socket(server_info->ai_family, server_info->ai_socktype,
			   server_info->ai_protocol)) == -1) {
      perror("Server: socket\n");
      exit(1);
    }

    // Clear addresses to avoid "address already in use" error.
    if (setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
      perror("setsockopt");
      exit(1);
    }

    // Bind the socket to the port.
    if (bind(socketfd, server_info->ai_addr, server_info->ai_addrlen) == -1) {
      perror("Server bind");
      exit(1);
    }

    freeaddrinfo(server_info);

    // Step 4: Listen on the socket
    listen(socketfd, BACKLOG_SIZE);
    addr_size = sizeof(client_addr_storage);
    while (1) {
      printf("Waiting for client connection...\n");
      client_socketfd = accept(socketfd, (struct sockaddr *)&client_addr_storage, &addr_size);
      printf("Client connected.");
      // Step 5: Handle clients that connect
      SendAck(client_socketfd);
      bytes_received = recv(client_socketfd, buffer, BUFFER_SIZE, 0);
      buffer[bytes_received] = '\0';
      printf("Query Received: %s \n", buffer);
      runQuery(client_socketfd, buffer);
    }
    // Step 6: Close the socket
    close(socketfd);

    Cleanup();

    return 0;
  }
}
