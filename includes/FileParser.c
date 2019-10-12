/*
 *  Created by Adrienne Slaughter
 *  CS 5007 Spring 2019
 *  Northeastern University, Seattle
 *
 *  This is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  It is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  See <http://www.gnu.org/licenses/>.
 *
 *  Edited by: Andrew Truong
 *  Date: 4/1/2019
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>

#include "MovieIndex.h"
#include "FileParser.h"
#include "Movie.h"
#include "DocIdMap.h"

void IndexTheFile(char *file, uint64_t docId, Index index);


// Returns a LinkedList of Movie structs from the specified file
LinkedList ReadFile(const char* filename){
  FILE *cfPtr;
  
  LinkedList movie_list = CreateLinkedList();

  if ((cfPtr = fopen(filename, "r")) == NULL) {
    printf("File could not be opened\n");
    DestroyLinkedList(movie_list, NULL);
    return NULL;
  } else {
    int max_row_length = 1000;
    char row[max_row_length];

    int i = 0; 

    
    while (fgets(row, max_row_length, cfPtr) != NULL) {
      i++; 
      // Got the line; create a movie from it
      MoviePtr movie = CreateMovieFromRow(row);
      if (movie != NULL) {
        InsertLinkedList(movie_list, movie);
      }
    }
    fclose(cfPtr);
    printf("Read %d rows, and have %d movies in the list\n", i, NumElementsInLinkedList(movie_list)); 
  }
  return movie_list;
}

/**
 * \fn Parses the files that are in a provided DocIdMap.
 *
 */
int ParseTheFiles(DocIdMap docs, Index index) {
  clock_t start2, end2;
  double cpu_time_used;

  start2 = clock();

  HTIter iter = CreateHashtableIterator(docs);

  int i = 0;

  HTKeyValue kv;

  while (HTIteratorHasMore(iter) != 0) {
    HTIteratorGet(iter, &kv);
    printf("processing file: %d\n", i++);
    IndexTheFile(kv.value, kv.key, index);
    HTIteratorNext(iter);
  }

  HTIteratorGet(iter, &kv);
  printf("processing file: %d\n", i++);
  IndexTheFile(kv.value, kv.key, index);

  DestroyHashtableIterator(iter);

  end2 = clock();
  cpu_time_used = ((double) (end2 - start2)) / CLOCKS_PER_SEC;

  printf("Took %f seconds to execute. \n", cpu_time_used);
  return 0;
}


void IndexTheFile(char *file, uint64_t doc_id, Index index) {
  FILE *cfPtr;

  if ((cfPtr = fopen(file, "r")) == NULL) {
    printf("File could not be opened\n");
    return;
  } else {
    int buffer_size = 1000;
    char buffer[buffer_size];
    int row = 0;

    while (fgets(buffer, buffer_size, cfPtr) != NULL) {
      Movie *movie = CreateMovieFromRow(buffer);
      int result = AddMovieTitleToIndex(index, movie, doc_id, row);
      if (result < 0) {
        fprintf(stderr, "Didn't add MovieToIndex.\n");
      }
      row++;
      DestroyMovie(movie);  // Done with this now
    }
    fclose(cfPtr);
  }
}

// Takes a linkedlist of movies, and builds a hashtable based on the given field
Index BuildMovieIndex(LinkedList movies, enum IndexField field_to_index){
  // TODO: How big to make this hashtable? How to decide? What to think about?
  // Make this "appropriate".
  if (NumElementsInLinkedList(movies) == 0) {
    return NULL;
  }
  Index movie_index = CreateIndex();
  movie_index->movies = movies;
  
  LLIter iter = CreateLLIter(movies);
  Movie* cur_movie;

  do {
    if (LLIterGetPayload(iter,(void**)&cur_movie) == 0) {
      AddMovieToIndex(movie_index, cur_movie, field_to_index); // Restart Here                                   
    }
  } while (LLIterNext(iter) == 0);

  // TODO: Check that there is at least one movie
  // What happens if there is not at least one movie?
  // How can we modify a piece(s) of our system to not have to do this check?

  DestroyLLIter(iter);
  
  return movie_index;
}

