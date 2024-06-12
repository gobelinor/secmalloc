#ifndef __SECMALLOC_PRIVATE_H__
#define __SECMALLOC_PRIVATE_H__
#include <stddef.h>
// #include "secmalloc.h"

// void *heapdata = NULL;
// struct chunkmetadata *heapmetadata = NULL;
// size_t heap_size = 4096;

extern void *heapdata;
extern struct chunkmetadata *heapmetadata;
extern size_t pageheap_size; // used as constant
extern size_t heapdata_size; // will increase
extern size_t heapmetadata_size; // will increase

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
long generate_canary();
size_t get_heapmetadata_size();
size_t get_heapdata_size();
void resizeheapmetadata();
void resizeheapdata();
struct chunkmetadata *lookup(size_t size);
void split(struct chunkmetadata *bloc, size_t size);
void place_canary(struct chunkmetadata *bloc, long canary);
// void *my_malloc(size_t size);
int is_valid(void *ptr);
int verify_canary(struct chunkmetadata *item);
void clean_memory(struct chunkmetadata *item);
void merge_chunks();
//void my_free(void *ptr);

#endif // __SECMALLOC_H__

