#ifndef __SECMALLOC_PRIVATE_H__
#define __SECMALLOC_PRIVATE_H__
#include <stddef.h>
#include "secmalloc.h"

extern size_t heap_size;

enum chunk_type 
{
	FREE = 0,
	BUSY = 1
};

// Struct to define metadata for a memory chunk
struct chunkmetadata
{
	size_t size;                   // Size of the chunk
	enum chunk_type flags;         // Flag indicating if the chunk is free or busy
	void *addr;                    // Address of the chunk
	long canary;                   // Canary value for detecting buffer overflows
	struct chunkmetadata *next;    // Pointer to the next chunk in the linked list
	struct chunkmetadata *prev;    // Pointer to the previous chunk in the linked list
};

// Struct to define a memory chunk
struct chunk 
{
	size_t size;
	enum chunk_type flags; // Flag indicating if the chunk is free or busy
};


// Function to initialize the heap data
void *init_heapdata(void);

// Function to initialize the heap metadata
struct chunkmetadata *init_heapmetadata(void);

// Function to initialize the heap
struct chunk *init_heap(void);

// Function to generate a random canary value
long generate_canary(void);

// Function to get a free chunk of the specified size
struct chunk *get_free_chunk(size_t size);

// Function to get a free chunk without any additional checks
struct chunk *get_free_chunk_raw(size_t size);

// Function to get the last raw chunk in the heap
struct chunk *get_last_chunk_raw(void);

// Function to clean and free a specified chunk
void clean(void *ptr);

// Function to look up a chunk of the specified size
struct chunkmetadata *lookup(size_t size);

#endif

