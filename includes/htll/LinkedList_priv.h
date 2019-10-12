// Inspired by UW CSE 333; used with permission.
//
// This is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published
// by the Free Software Foundation, either version 3 of the License,
// or (at your option) any later version.
// It is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// Header file containing structs and functions used to implement a
// doubly linked list ADT.
//
// Author: Andrew Truong
// Date: 3/17/2019


#include <stdint.h>      // for uint64_t
#include "./LinkedList.h"  // for LinkedList and LLIter

// This file defines the internal structures associated with our LinkedList
// implementation.  Customers should not include this file or assume anything
// based on its contents.  Instead, we have broken these out into this file so
// that the unit test code for LinkedList has access to it, allowing unit
// tests to peek inside the implementation to check pointers and fields for
// correctness.

// This struct represents an individual node within a linked list.  A node
// contains next and prev pointers as well as a customer-supplied payload
// pointer.

#ifndef LINKEDLIST_PRIV_H
#define LINKEDLIST_PRIV_H

// This struct represents an individual node within a linked list.  A node
// contains next and prev pointers as well as a customer-supplied payload
// pointer.
typedef struct ll_node {
    void *payload;  // customer-supplied payload pointer
    struct ll_node *next;     // next node in list, or NULL
    struct ll_node *prev;     // prev node in list, or NULL
} LinkedListNode, *LinkedListNodePtr;

// This struct represents the entire linked list.  We provided a struct
// declaration (but not definition) in LinkedList.h; this is the associated
// definition.  This struct contains metadata about the linked list.
typedef struct ll_head {
    uint64_t num_elements;  //  # elements in the list
    LinkedListNodePtr head;  // head of linked list, or NULL if empty
    LinkedListNodePtr tail;  // tail of linked list, or NULL if empty
} LinkedListHead;

// This struct represents the state of an iterator.  We expose the struct
// declaration in LinkedList.h, but not the definition, similar to what we did
// above for the linked list itself.
typedef struct ll_iter {
    LinkedList list;  // the list we're for
    LinkedListNodePtr cur_node;  // the node we are at, or NULL if broken
} LLIterSt;

// Creates a LinkedListNode by malloc'ing the space.
//
// INPUT: A pointer that the payload of the returned LLNode will point to.
//
// Returns a pointer to the new LinkedListNode.
LinkedListNode *CreateLinkedListNode(void *data);

// Destroys and free's a provided LLNode.
//
// INPUT: A pointer to the node to destroy.
//
// Returns 0 if the destroy was successful.
int DestroyLinkedListNode(LinkedListNode *node);

// Removes a given element from a linkedList.
//
// INPUT: A pointer to a linked list.
// INPUT: A ListNodePtr that points to a LLNode to be removed from the list.
//
// Returns 0 if the destroy was successful
//   (primarily that the provide Ptr is in the list and could be free'd).
int RemoveLLElem(LinkedList list, LinkedListNodePtr ptr);

#endif  //LINKEDLIST_PRIV_H
