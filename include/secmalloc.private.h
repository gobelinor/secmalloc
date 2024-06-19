#ifndef SECMALLOC_PRIVATE_H
#define SECMALLOC_PRIVATE_H

#include <stddef.h>

#define PAGE_HEAP_SIZE       4096 // used as constant
#define MAX_METADATA_SIZE    (100000 * sizeof(struct chunkmetadata))
#define BASE_ADDRESS         ((void*)(4096 * 1000))

/**
 * @file secmalloc_private.h
 * @brief Header file for private secure memory allocation functions.
 *
 * This file contains the private declarations and definitions for secure memory allocation
 * functions used throughout the project.
 */

extern void                    *heapdata; ///< Pointer to the heap data
extern struct chunkmetadata    *heapmetadata; ///< Pointer to the heap metadata
extern size_t                  heapdata_size; ///< Size of the heap data
extern size_t                  heapmetadata_size; ///< Size of the heap metadata

/**
 * @brief Enum to define the chunk types.
 */
enum chunk_type
{
    FREE = 0, ///< Chunk is free
    BUSY = 1  ///< Chunk is busy
};

/**
 * @brief Struct to define metadata for a memory chunk.
 */
struct chunkmetadata
{
    size_t                  size;                   ///< Size of the chunk
    enum                    chunk_type flags;         ///< Flag indicating if the chunk is free or busy
    void                    *addr;                    ///< Address of the chunk
    long                    canary;                   ///< Canary value for detecting buffer overflows
    struct chunkmetadata    *next;    ///< Pointer to the next chunk in the linked list
};

/**
 * @brief Function to initialize the heap data.
 *
 * @return void* A pointer to the initialized heap data.
 */
void    *my_init_heapdata();

/**
 * @brief Function to initialize the heap metadata.
 *
 * @return struct chunkmetadata* A pointer to the initialized heap metadata.
 */
struct chunkmetadata    *my_init_heapmetadata();

/**
 * @brief Function to generate a random canary value.
 *
 * @return long The generated canary value.
 */
long    my_generate_canary();

/**
 * @brief Function to get the total allocated size of the heap metadata.
 *
 * @return size_t The total allocated size of the heap metadata.
 */
size_t    my_get_allocated_heapmetadata_size();

/**
 * @brief Function to get the last metadata block.
 *
 * @return struct chunkmetadata* A pointer to the last metadata block.
 */
struct chunkmetadata   *my_lastmetadata();

/**
 * @brief Function to resize the heap metadata.
 */
void    my_resizeheapmetadata();

/**
 * @brief Function to resize the heap data.
 */
void    my_resizeheapdata();

/**
 * @brief Function to look up a free block with enough size.
 *
 * @param size The size required for the block.
 * @return struct chunkmetadata* A pointer to the found free block, or NULL if no block is found.
 */
struct chunkmetadata    *my_lookup(size_t size);

/**
 * @brief Function to split a block into two blocks.
 *
 * @param bloc The block to split.
 * @param size The size of the first block after the split.
 * @param canary The canary value to place in the block.
 */
void    my_split(struct chunkmetadata *bloc, size_t size, long canary);

/**
 * @brief Function to place a canary at the end of a block.
 *
 * @param bloc The block to place the canary in.
 * @param canary The canary value to place.
 */
void    my_place_canary(struct chunkmetadata *bloc, long canary);

/**
 * @brief Function to verify the canary value of a block.
 *
 * @param item The block to verify.
 * @return int 1 if the canary value is valid, 0 otherwise.
 */
int    my_verify_canary(struct chunkmetadata *item);

/**
 * @brief Function to clean the memory of a block.
 *
 * @param item The block to clean.
 */
void    my_clean_memory(struct chunkmetadata *item);

/**
 * @brief Function to merge consecutive free chunks.
 */
void    my_merge_chunks(void);

#endif // SECMALLOC_PRIVATE_H
