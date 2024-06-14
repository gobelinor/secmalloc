#ifndef __SECMALLOC_PRIVATE_H__
#define __SECMALLOC_PRIVATE_H__
#include <stddef.h>

extern void *heapdata;
extern struct chunkmetadata *heapmetadata;
#define PAGE_HEAP_SIZE 4096 // used as constant
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
	struct chunkmetadata *prev;    // Pointer to the previous chunk in the linked list
};

// Struct to define a memory chunk
struct chunk
{
	size_t size;
	enum chunk_type flags; // Flag indicating if the chunk is free or busy
};

// Function to initialize the heap data
void *init_heapdata();

// Function to initialize the heap metadata
struct chunkmetadata *init_heapmetadata();

// Function to generate a random canary value
long generate_canary();

// Function to get the total size of the heap metadata
size_t get_heapmetadata_size();

// Function to get the total size of the heap data
size_t get_heapdata_size();

// Function to resize the heap metadata
void resizeheapmetadata();

// Function to resize the heap data
void resizeheapdata();

// Function to look up a free block with enough size
struct chunkmetadata *lookup(size_t size);

// Function to split a block into two, returning the new second block
void split(struct chunkmetadata *bloc, size_t size);

// Function to place a canary at the end of a block
void place_canary(struct chunkmetadata *bloc, long canary);

// Function to check if a pointer is valid (not used, included in free)
int is_valid(void *ptr);

// Function to verify the canary value of a block
int verify_canary(struct chunkmetadata *item);

// Function to clean the memory of a block
void clean_memory(struct chunkmetadata *item);

// Function to merge consecutive free chunks
void merge_chunks();

#endif // __SECMALLOC_H__

