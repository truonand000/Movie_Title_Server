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
 *  Edited By: Andrew Truong
 *  Date: 4/1/2019 
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "MovieIndex.h"
#include "htll/LinkedList.h"
#include "htll/Hashtable.h"
#include "Movie.h"
#include "MovieSet.h"

// Prototype of function that looks through a linked list
// And destroys any doubles.
void SeekAndDestroyDuplicates(LinkedList, Movie *);

void DestroyMovieSetWrapper(void *movie_set) {
  DestroyMovieSet((MovieSet)movie_set);
}

void DestroySetOfMovieWrapper(void *set_movie) {
  DestroySetOfMovies((SetOfMovies)set_movie);
}

void toLower(char *str, int len) {
  for (int i = 0; i < len; i++) {
    str[i] = tolower(str[i]);
  }

}


Index CreateIndex() {
  Index ind = (Index)malloc(sizeof(struct index));
  // TODO: How big to make this hashtable? How to decide? What to think about?
  // Make this "appropriate".
  ind->ht = CreateHashtable(128);
  ind->movies = NULL; // TO BE NULL until it's populated/used.
  return ind;
}

int DestroyIndex(Index index, void (*destroyValue)(void *)) {
  DestroyHashtable(index->ht, destroyValue);

  if (index->movies != NULL) {
    DestroyLinkedList(index->movies, DestroyMovieWrapper);
  }
  free(index);
  return 0;
}

// Destroy index that has an offsetlist as a value
int DestroyOffsetIndex(Index index) {
    return DestroyIndex(index, DestroyMovieSetWrapper);
}

// Destroy's index that has aSetOfMovies as a value
int DestroyTypeIndex(Index index) {
  return DestroyIndex(index, DestroySetOfMovieWrapper);
}


// Assumes Index is a hashtable with key=title word, and value=hashtable with key doc id and value linked list of rows
int AddMovieTitleToIndex(Index index,
                         Movie *movie,
                         uint64_t doc_id,
                         int row_id) {
  // Put in the index
  HTKeyValue kvp;

  // TODO: How to choose this number? What are the pros and cons of choosing the wrong number?
  int numFields = 1000;

  char *token[numFields];
  char *rest = movie->title;

  int i = 0;
  token[i] = strtok_r(rest, " ", &rest);
  while (token[i] != NULL) {
    toLower(token[i], strlen(token[i]));
    i++;
    token[i] = strtok_r(rest, " ", &rest);
  }

  for (int j = 0; j < i; j++) {
    // If this key is already in the hashtable, get the MovieSet.
    // Otherwise, create a MovieSet and put it in.
    int result = LookupInHashtable(index->ht,
                          FNVHash64((unsigned char*)token[j],
                                    (unsigned int)strlen(token[j])), &kvp);
    HTKeyValue old_kvp;

    if (result < 0) {
      kvp.value = CreateMovieSet(token[j]);
      kvp.key = FNVHash64((unsigned char*)token[j], strlen(token[j]));
      PutInHashtable(index->ht, kvp, &old_kvp);
    }

    AddMovieToSet((MovieSet)kvp.value, doc_id, row_id);
  }

  return 0;

}


// Adds the movie to the index all by genre
int AddMovieToIndex_Genre(Index index, Movie *movie) {

  // Put in the index
  HTKeyValue kvp;
  HTKeyValue old_kvp;

  for (int i=0; i < NUM_GENRES; i++) {
    char* genre = movie->genres[i];
    if (genre == NULL) {
      return 0;
    }
    uint64_t genre_key = FNVHash64((unsigned char*)genre, strlen(genre));

    // If this key is already in the hashtable, get the SetOfMovies.
    // Otherwise, create a SetOfMovies and put it in.
    int result = LookupInHashtable(index->ht,
                                   genre_key,
                                   &kvp);

    if (result < 0) {
      kvp.value = CreateSetOfMovies(genre);  // Should be something like "Documentary",
      kvp.key = genre_key;
      PutInHashtable(index->ht, kvp, &old_kvp);
    }
    // Check to see if there is a movie of the same title.
    // If there is, then destroy it.
    SetOfMovies check_set_movies = (SetOfMovies)kvp.value;
    SeekAndDestroyDuplicates(check_set_movies->movies, movie);
    AddMovieToSetOfMovies((SetOfMovies)kvp.value, movie);
  }

  return 0;
}

// Assumes index is a hashtable with key=string of the field, and value is a SetOfMovies
int AddMovieToIndex(Index index, Movie *movie, enum IndexField field) {
  if (field == Genre) {
    return AddMovieToIndex_Genre(index, movie);
  }
  // Put in the index
  HTKeyValue kvp;
  HTKeyValue old_kvp;

  // If this key is already in the hashtable, get the MovieSet.
  // Otherwise, create a MovieSet and put it in.
  int result = LookupInHashtable(index->ht,
                                 ComputeKey(movie, field),
                                 &kvp);

  if (result < 0) {
    char* doc_set_name;
    char year_str[10];
    switch (field) {
      case Type:
        doc_set_name = movie->type;
        break;
      case Year:
        snprintf(year_str, sizeof(year_str), "%d", movie->year);
        doc_set_name = year_str;
        break;
      case Id:
        doc_set_name = movie->id;
	break;
    case Genre:
      return 0;
    }
    kvp.value = CreateSetOfMovies(doc_set_name);  // Should be something like "1974", or "Documentary"
    kvp.key = ComputeKey(movie, field);
    PutInHashtable(index->ht, kvp, &old_kvp);
  }

  // TODO: How do we put the movie in the index?
  SetOfMovies check_set_movies = (SetOfMovies)kvp.value;
  SeekAndDestroyDuplicates(check_set_movies->movies, movie);
  AddMovieToSetOfMovies((SetOfMovies)kvp.value, movie);
  return 0;
}





uint64_t ComputeKey(Movie* movie, enum IndexField which_field) {
  switch(which_field) {
    case Year:
      return FNVHashInt64(movie->year);
      break;
    case Type:
      return FNVHash64((unsigned char*)movie->type, strlen(movie->type));
      break;
    case Id:
      return FNVHash64((unsigned char*)movie->id, strlen(movie->id));
      break;
  case Genre:
    return -1u;
    break;
  }
  return -1u;
}


MovieSet GetMovieSet(Index index, const char *term) {
  HTKeyValue kvp;
  char lower[strlen(term)+1];
  strcpy(lower, term);
  toLower(lower, strlen(lower));
  int result = LookupInHashtable(index->ht,
                                 FNVHash64((unsigned char*)lower,
                                           (unsigned int)strlen(lower)),
                                 &kvp);
  if (result < 0) {
    printf("term couldn't be found: %s \n", term);
    return NULL;
  }
  printf("returning movieset\n");
  return (MovieSet)kvp.value;
}

// Function used to seek doubles within the same key chain and destroy the old
// key to make room for the new key and value pair.
void SeekAndDestroyDuplicates(LinkedList movie_list, Movie *movie) {
  Movie *current_movie;
  if (NumElementsInLinkedList(movie_list) > 0) {
    LLIter destroy_search_iter =  CreateLLIter(movie_list);
    do {
      if (LLIterGetPayload(destroy_search_iter, (void**)&current_movie) == 0) {
	// printf("Current Movie: %s\n", current_movie->title);
	if (strcmp(current_movie->title, movie->title) == 0) {
	  // printf("Deleted %s due to same title as %s\n", current_movie->title, movie->title);
	  LLIterDelete(destroy_search_iter, &NullFree);
	  break;
	}
      }
    } while (LLIterNext(destroy_search_iter) == 0);
    DestroyLLIter(destroy_search_iter);
  }
}

