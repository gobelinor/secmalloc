#ifndef __SECMALLOC_PRIVATE_H__
#define __SECMALLOC_PRIVATE_H__
#include <stddef.h>

#define PAGE_HEAP_SIZE 4096 // used as constant
#define MAX_METADATA_SIZE (100000 * sizeof(struct chunkmetadata))
#define BASE_ADDRESS ((void*)(4096 * 1000))


extern void *heapdata;
extern struct chunkmetadata *heapmetadata;
// #define PAGE_HEAP_SIZE 4096 // used as constant
extern size_t heapdata_size; // will increase
extern size_t heapmetadata_size; // will increase

//
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
};

// Function to initialize the heap data
void *init_heapdata();

// Function to initialize the heap metadata
struct chunkmetadata *init_heapmetadata();

// Function to generate a random canary value
long generate_canary();

// Function to get the total alocated size of the heap metadata
size_t get_allocated_heapmetadata_size();

// Function to get the last metadata bloc
struct chunkmetadata *lastmetadata();

// Function to resize the heap metadata
void resizeheapmetadata();

// Function to resize the heap data
void resizeheapdata();

// Function to look up a free block with enough size
struct chunkmetadata *lookup(size_t size);

// Function to split a block into two blocks
void split(struct chunkmetadata *bloc, size_t size, long canary);

// Function to place a canary at the end of a block
void place_canary(struct chunkmetadata *bloc, long canary);

// Function to verify the canary value of a block
int verify_canary(struct chunkmetadata *item);

// Function to clean the memory of a block
void clean_memory(struct chunkmetadata *item);

// Function to merge consecutive free chunks
void merge_chunks();

#endif // __SECMALLOC_H__

