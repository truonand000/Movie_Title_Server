
#include <stdio.h>
#include <stdlib.h>

#include "MovieIndex.h"
#include "MovieReport.h"
#include "Movie.h"
#include "MovieSet.h"
#include "htll/LinkedList.h"
#include "htll/Hashtable.h"


// Prints a report of an index, assuming the key is a field/string and the value is a SetOfMovies.
void PrintReport(Index index) {
  HTIter indexIter = CreateHashtableIterator(index->ht);
  HTKeyValue payload;
  Movie *movie;
  do {
    if (HTIteratorGet(indexIter, &payload) == 0) {
      SetOfMovies set_of_movies = (SetOfMovies)payload.value;
      printf("indexType: %s\n", set_of_movies->desc);
      printf("%d items\n", NumElementsInLinkedList(set_of_movies->movies));
      LLIter ll_iter = CreateLLIter(set_of_movies->movies);
      do {
	if (LLIterGetPayload(ll_iter,(void**)&movie) == 0) {
	  printf("\t%s\n", movie->title);
	}
      } while (LLIterNext(ll_iter) == 0);
      DestroyLLIter(ll_iter);
    }
    if (!HTIteratorHasMore(indexIter)) {
      break;
    }
  } while(HTIteratorNext(indexIter) == 0);

  DestroyHashtableIterator(indexIter);
}

void OutputListOfMovies(LinkedList movie_list, char *desc, FILE *file) {
  fprintf(file, "%s: %s\n", "indexType", desc);
  fprintf(file, "%d items\n", NumElementsInLinkedList(movie_list));
  LLIter iter = CreateLLIter(movie_list);
  if (iter == NULL) {
    fprintf(file, "iter null for some reason.. \n");
    return;
  }
  Movie *movie;

  LLIterGetPayload(iter, (void**)&movie);
  if (movie->title != NULL) fprintf(file, "\t%s\n", movie->title);
  else fprintf(file, "title is null\n");
 
  while (LLIterHasNext(iter)) {
    LLIterNext(iter);
    LLIterGetPayload(iter, (void**)&movie);
    if (movie->title != NULL) fprintf(file, "\t%s\n", movie->title);
    else fprintf(file, "title is null\n");
  }

  DestroyLLIter(iter);
}

// Assumes the value of the index is a SetOfMovies
void OutputReport(Index index, FILE* output) {
  // TODO: Implement this function.
  // After you've implemented it, consider modifying PrintReport()
  // to utilize this function.

}

void SaveReport(Index index, char* filename) {
  // TODO: Implement this. You might utilize OutputReport.

  
}

