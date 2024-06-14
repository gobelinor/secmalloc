#ifndef __MY_ALLOC_PRIVATE_H__
#define __MY_ALLOC_PRIVATE_H__

#include <stddef.h>
#include "my_alloc.h"
extern size_t heap_size;

// Enum to define chunk types
enum chunk_type 
{
	FREE = 0,
	BUSY = 1
};

// Struct to define a memory chunk
struct chunk 
{
	size_t size;
	enum chunk_type flags; // Flag indicating if the chunk is free or busy
};

// Function to initialize the heap
struct chunk *init_heap(void);

// Function to get a free chunk of the specified size
struct chunk *get_free_chunk(size_t size);

// Function to get a free chunk without any additional checks
struct chunk *get_free_chunk_raw(size_t size);

// Function to get the last raw chunk in the heap
struct chunk *get_last_chunk_raw(void);

// Function to look up a chunk of the specified size
struct chunk *lookup(size_t size);

// Function to clean and free a specified chunk
void clean(void *ptr);

#endif

