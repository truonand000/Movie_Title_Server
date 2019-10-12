// CS 5007, Northeastern University, Seattle
// Summer 2017
//
// Assignment 2
#include "Hashtable.h"

#ifndef HASHTABLE_PRIV_H
#define HASHTABLE_PRIV_H


// This is the struct we use to represent an iterator.
typedef struct ht_itrec {
  Hashtable  ht;          // the HT we're pointing into
  int   which_bucket;  // which bucket are we in?
  LLIter bucket_iter;   // iterator for the bucket, or NULL
} HTIterRecord;


// uint64_t GetFirstElementKey(LinkedList list);

void ResizeHashtable(Hashtable ht);

int HashKeyToBucketNum(Hashtable ht, uint64_t key); 

//typedef struct hashtableInfo HashtableInfo;

//int BucketHasKey(LLIter iter, uint64_t key, HTKeyValue **keyvalue);

double GetAlpha(Hashtable *hashtable);

double Resize(Hashtable *hashtable); 

// double ComputeHash(key);

int Rehash(); 

//int 


#endif // HASHTABLE_PRIV_H
