#ifndef __SECMALLOC_PRIVATE_H__
#define __SECMALLOC_PRIVATE_H__
#include <stddef.h>
#include "secmalloc.h"

// void *heapdata = NULL;
// struct chunkmetadata *heapmetadata = NULL;
// size_t heap_size = 4096;
//
enum chunk_type 
{
	FREE = 0,
	BUSY = 1
};

struct chunkmetadata 
{
	size_t size;
	enum chunk_type flags;
	void *addr;
	long canary;
	struct chunkmetadata *next;
	struct chunkmetadata *prev;
};

struct chunk 
{
	size_t size;
	enum chunk_type flags;
};

void *init_heapdata();
struct chunkmetadata *init_heapmetadata();
struct chunk *init_heap();
long generate_canary();
struct chunk *get_free_chunk(size_t size);
struct chunk *get_free_chunk_raw(size_t size);
struct chunk *get_last_chunk_raw();
void clean(void *ptr);

#endif // __SECMALLOC_H__

