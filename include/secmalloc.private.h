#ifndef __MY_ALLOC_PRIVATE_H__
#define __MY_ALLOC_PRIVATE_H__
#include <stddef.h>
#include "my_alloc.h"

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
	uint64_t canary;
	struct chunkmetadata *next;
	struct chunkmetadata *prev;
};

struct chunk 
{
	size_t size;
	enum chunk_type flags;
};

struct chunkmetadata *init_heaps();
struct chunk *init_heap();
struct chunk *get_free_chunk(size_t size);
struct chunk *get_free_chunk_raw(size_t size);
struct chunk *get_last_chunk_raw();
void clean(void *ptr);

#endif // __MY_ALLOC_H__

