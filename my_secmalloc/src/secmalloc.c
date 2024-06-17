/**
 * @file secmalloc.c
 * @brief Implementation of secure memory allocation functions.
 *
 * This file contains the implementation of secure memory allocation
 * functions including initialization, allocation, deallocation, and
 * management of heap metadata.
 */

#include "secmalloc.h"
#define _GNU_SOURCE
#include <fcntl.h>
#include <stdio.h>
#include <sys/mman.h>
#include <linux/mman.h>
#include <unistd.h>
#include <string.h>
#include "log.h"

// Global variables
void                    *heapdata = NULL; // Pointer to the heap data
struct chunkmetadata    *heapmetadata = NULL; // Pointer to the heap metadata
size_t                  heapdata_size = PAGE_HEAP_SIZE; // Current size of the heap data, will increase as needed
size_t                  heapmetadata_size = PAGE_HEAP_SIZE; // Current size of the heap metadata, will increase as needed

/**
 * @brief Initialize heap data.
 *
 * This function initializes the heap data by mapping memory.
 *
 * @return void* A pointer to the initialized heap data, or NULL if the initialization fails.
 */
void* my_init_heapdata()
{
    my_log_message("call init_heapdata\n");
    if (heapdata == NULL)
    {
        // Attempt to map memory for heap data
        heapdata = mmap((void*)((size_t)BASE_ADDRESS + MAX_METADATA_SIZE), PAGE_HEAP_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

        // Check if the mmap operation was successful
        if (heapdata == MAP_FAILED) {
            perror("mmap");
            my_log_message("Error: Failed to mmap memory for heap data.\n");
            return NULL;
        }
    }
    my_log_message("return heapdata %p\n", heapdata);
    return heapdata;
}

/**
 * @brief Initialize heap metadata.
 *
 * This function initializes the heap metadata by mapping memory.
 *
 * @return struct chunkmetadata* A pointer to the initialized heap metadata, or NULL if the initialization fails.
 */
struct chunkmetadata* my_init_heapmetadata()
{
    my_log_message("call init_heapmetadata\n");
    if (heapmetadata == NULL)
    {
        // Attempt to map memory for heap metadata
        heapmetadata = (struct chunkmetadata*) mmap(BASE_ADDRESS, PAGE_HEAP_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

        // Check if the mmap operation was successful
        if (heapmetadata == MAP_FAILED) {
            perror("mmap");
            my_log_message("Error: Failed to mmap memory for heap metadata.\n");
            return NULL;
        }

        // Initialize the first chunk metadata
        heapmetadata->size = PAGE_HEAP_SIZE;
        heapmetadata->flags = FREE;
        heapmetadata->addr = heapdata;
        heapmetadata->canary = 0xdeadbeef; // will be replaced by a random value during first malloc
        heapmetadata->next = NULL;
    }
    my_log_message("return heapmetadata %p\n", heapmetadata);
    return heapmetadata;
}

/**
 * @brief Generate a random canary value.
 *
 * This function generates a random canary value using /dev/urandom.
 *
 * @return long The generated canary value, or -1 if the generation fails.
 */
long my_generate_canary()
{
    my_log_message("call generate_canary\n");
    long    canary = 0;
    int     fd = open("/dev/urandom", O_RDONLY);

    // Check if opening /dev/urandom was successful
    if (fd == -1)
    {
        perror("open");
        my_log_message("Error: Failed to open /dev/urandom for canary generation.\n");
        return -1;
    }

    // Read random data into the canary variable
    ssize_t    result = read(fd, &canary, sizeof(long));
    if (result == -1)
    {
        perror("read");
        my_log_message("Error: Failed to read from /dev/urandom for canary generation.\n");
        close(fd); // Close the file descriptor before returning
        return -1;
    }
    else if (result != sizeof(long))
    {
        my_log_message("Error: Incomplete read from /dev/urandom. Expected %zu bytes but got %zd bytes.\n", sizeof(long), result);
        close(fd); // Close the file descriptor before returning
        return -1;
    }

    close(fd); // Close the file descriptor after reading
    my_log_message("return canary %ld\n", canary);
    return canary;
}

/**
 * @brief Get the total allocated size of the heap metadata.
 *
 * This function calculates the total allocated size of the heap metadata.
 *
 * @return size_t The total allocated size of the heap metadata.
 */
size_t my_get_allocated_heapmetadata_size()
{
    my_log_message("call get_allocated_heapmetadata_size\n");
    size_t    size = 0;
    void      *item = heapmetadata;

    while (((struct chunkmetadata*)item)->size != 0)
    {
        size += sizeof(struct chunkmetadata);
        item = (void*)((size_t)item + sizeof(struct chunkmetadata));
    }

    my_log_message("return size %zu\n", size);
    return size;
}

/**
 * @brief Get the total allocated size of the heap data.
 *
 * This function calculates the total allocated size of the heap data.
 *
 * @return size_t The total allocated size of the heap data.
 */
size_t my_get_allocated_heapdata_size()
{
    my_log_message("call get_allocated_heapdata_size\n");
    struct chunkmetadata    *last_item = NULL;
    size_t                  size = 0;

    for (struct chunkmetadata *item = heapmetadata; item != NULL; item = item->next)
    {
        size += item->size + sizeof(long); // add the size of the canary
        last_item = item;
    }

    if (last_item->flags == FREE)
    {
        size -= last_item->size;
    }

    my_log_message("return size %zu\n", size);
    return size;
}

/**
 * @brief Get the last metadata block.
 *
 * This function returns the last metadata block in the linked list.
 *
 * @return struct chunkmetadata* A pointer to the last metadata block.
 */
struct chunkmetadata* my_lastmetadata()
{
    my_log_message("call lastmetadata\n");
    struct chunkmetadata    *item = heapmetadata;

    while (item->next != NULL)
    {
        item = item->next;
    }

    my_log_message("Last metadata block at %p, size : %zu, flags : %d\n", item, item->size, item->flags);
    return item;
}

/**
 * @brief Resize the heap metadata.
 *
 * This function resizes the heap metadata when necessary.
 */
void my_resizeheapmetadata()
{
    my_log_message("call resizeheapmetadata\n");

    // Ensure the current heap metadata size is valid
    if (heapmetadata == NULL) {
        my_log_message("Error: Heap metadata is not initialized.\n");
        return;
    }

    void    *old_heapmetadata = heapmetadata;

    // Calculate the new size of the heap metadata
    size_t    new_size = heapmetadata_size + PAGE_HEAP_SIZE;

    // Attempt to resize the heap metadata using mremap
    void    *new_heapmetadata = mremap(heapmetadata, heapmetadata_size, new_size, MREMAP_MAYMOVE);

    // Check if the remapping was successful
    if (new_heapmetadata == MAP_FAILED) {
        perror("mremap");
        my_log_message("Error: Failed to resize heap metadata.\n");
        return;
    }

    if (old_heapmetadata != new_heapmetadata){
        my_log_message("Error: old_heapmetadata != new_heapmetadata\n");
        my_log_message("old_heapmetadata : %p\n", old_heapmetadata);
        my_log_message("new_heapmetadata : %p\n", new_heapmetadata);
    }

    // Update the heap metadata pointer and size
    heapmetadata = new_heapmetadata;
    heapmetadata_size = new_size;

    my_log_message("new heapmetadata size %zu\n", heapmetadata_size);
    return;
}

/**
 * @brief Resize the heap data.
 *
 * This function resizes the heap data to the specified new size.
 *
 * @param new_size The new size for the heap data.
 */
void my_resizeheapdata(size_t new_size)
{
    my_log_message("call resizeheapdata\n");

    // Ensure the current heap data size is valid
    if (heapdata == NULL) {
        my_log_message("Error: Heap data is not initialized.\n");
        return;
    }

    void    *old_heapdata = heapdata;
    // Attempt to resize the heap data using mremap
    void    *new_heapdata = mremap(heapdata, heapdata_size, new_size, MREMAP_MAYMOVE);

    if (old_heapdata != new_heapdata){
        my_log_message("Error: old_heapdata != new_heapdata\n");
        my_log_message("old_heapdata : %p\n", old_heapdata);
        my_log_message("new_heapdata : %p\n", new_heapdata);
    }

    // Check if the remapping was successful
    if (new_heapdata == MAP_FAILED) {
        perror("mremap");
        my_log_message("Error: Failed to resize heap data.\n");
        return;
    }

    // Update the heap data pointer and size
    heapdata = new_heapdata;
    heapdata_size = new_size;

    // Get the last metadata block
    struct chunkmetadata    *last = my_lastmetadata();
    if (last != NULL)
    {
        last->size = new_size;
    }
    else
    {
        my_log_message("Error: No metadata found to update the size.\n");
    }

    my_log_message("new heapdata size %zu\n", heapdata_size);
    return;
}

/**
 * @brief Look up a free block with enough size.
 *
 * This function looks up a free block in the heap metadata that is large enough to accommodate the requested size.
 *
 * @param size The size required for the block.
 * @return struct chunkmetadata* A pointer to the found free block, or NULL if no block is found.
 */
struct chunkmetadata* my_lookup(size_t size)
{
    my_log_message("call lookup\n");
    // Check if the heap metadata is initialized
    if (heapmetadata == NULL) {
        my_log_message("Error: Heap metadata is not initialized.\n");
        return NULL;
    }

    // Traverse the  linked list of chunkmetadata to find a suitable free block
    for (struct chunkmetadata *item = heapmetadata; item != NULL; item = item->next)
    {
        // Check if the current block is free and has enough size
        if (item->flags == FREE && item->size >= size + sizeof(long))
        {
            my_log_message("Found suitable free block %p pointing to %p of size %zu bytes.\n", item, item->addr, item->size);
            return item; // Return the suitable free block
        }
    }

    my_log_message("Error: No suitable free block found for size %zu bytes.\n", size);
    return NULL; // Return NULL if no suitable block is found
}

/**
 * @brief Split a block into two blocks.
 *
 * This function splits a given block into two blocks.
 *
 * @param bloc The block to split.
 * @param size The size of the first block after the split.
 * @param canary The canary value to place in the block.
 */
void my_split(struct chunkmetadata *bloc, size_t size, long canary)
{
    my_log_message("call split block %p pointing to %p of size %zu bytes into %zu bytes and %zu bytes.\n", bloc,  bloc->addr, bloc->size, size, bloc->size - size - sizeof(long));
    // Check if the block to be split is valid
    if (bloc == NULL) {
        my_log_message("Error: Attempted to split a NULL block.\n");
        return;
    }
    // Create new metadata block for the second part
    struct chunkmetadata    *newbloc = (struct chunkmetadata*) ((size_t)heapmetadata + my_get_allocated_heapmetadata_size());
    my_log_message("in split : selected empty new newbloc %p pointing to %p, size = %zu, flags = %d\n", newbloc, newbloc->addr, newbloc->size, newbloc->flags);

    // Set metadata for the new block
    newbloc->size = bloc->size - size - sizeof(long);
    newbloc->flags = FREE;
    newbloc->addr = (void*)((size_t)bloc->addr + size + sizeof(long));
    newbloc->canary = 0xdeadbeef;
    newbloc->next = bloc->next;

    // Set the metadata for the first block
    // bloc == newbloc should really not happen
    if (bloc == newbloc)
    {
        my_log_message("Error: bloc %p == newbloc %p\n", bloc, newbloc);
        bloc->next = newbloc->next;
    }
    else
    {
        // Should always be the case
        bloc->next = newbloc;
    }

    bloc->size = size;
    bloc->flags = BUSY;
    bloc->canary = canary;

    my_log_message("end split : newbloc %p pointing to %p, size = %zu, flags = %d, canary = %ld, next = %p\n", newbloc, newbloc->addr, newbloc->size, newbloc->flags, newbloc->canary, newbloc->next);
    return;
}

/**
 * @brief Place a canary at the end of a block.
 *
 * This function places a canary value at the end of the specified block.
 *
 * @param bloc The block to place the canary in.
 * @param canary The canary value to place.
 */
void my_place_canary(struct chunkmetadata *bloc, long canary)
{
    my_log_message("call place_canary\n");

    // Check if the block is valid
    if (bloc == NULL)
    {
        my_log_message("Error: Attempted to place canary in a NULL block.\n");
        return;
    }
    // Calculate the address where the canary should be placed
    long *canary_ptr = (long*)((size_t)bloc->addr + bloc->size);

    // Place the canary value at the calculated address
    *canary_ptr = canary;

    my_log_message("Canary placed at %p with value %ld\n", canary_ptr, *canary_ptr);
    return;
}

/**
 * @brief Allocate memory of the specified size.
 *
 * This function allocates memory of the specified size and returns a pointer to it.
 *
 * @param size The size of the memory block to allocate.
 * @return void* A pointer to the allocated memory, or NULL if the allocation fails.
 */
void* my_malloc(size_t size)
{
    my_log_message("\n\nCALL MALLOC SIZE %zu\n", size);

    // If requested size is 0, return NULL
    if (size == 0)
    {
        return NULL;
    }

    // Check if the heap data is initialized
    if (heapdata == NULL)
    {
        if (my_init_heapdata() == NULL)
        {
            return NULL; // Initialization failed
        }
    }

    // Check if the heap metadata is initialized
    if (heapmetadata == NULL)
    {
        if (my_init_heapmetadata() == NULL)
        {
            return NULL; // Initialization failed
        }
    }

    // Get the total size of allocated heap metadata and resize if needed
    size_t    allocated_heapmetadata_size = my_get_allocated_heapmetadata_size();
    if (PAGE_HEAP_SIZE - allocated_heapmetadata_size % PAGE_HEAP_SIZE < sizeof(struct chunkmetadata))
    {
        my_resizeheapmetadata();
    }

    // Get the total size of allocated data heap and resize if needed
    size_t    allocated_heapdata_size = my_get_allocated_heapdata_size();
    size_t    needed_size = size + sizeof(long);
    size_t    available_size = heapdata_size - allocated_heapdata_size;
    if (available_size < needed_size)
    {
        size_t    new_size = allocated_heapdata_size + needed_size;
        new_size = ((new_size / PAGE_HEAP_SIZE) + ((new_size % PAGE_HEAP_SIZE != 0) ? 1 : 0)) * PAGE_HEAP_SIZE;
        my_resizeheapdata(new_size);
    }

    // Look up a free block with large enough size
    struct chunkmetadata    *bloc = my_lookup(size);
    if (bloc == NULL)
    {
        return NULL; // No suitable block found
    }

    // Generate a canary
    long    canary = my_generate_canary();
    if (canary == -1)
    {
        return NULL; // Canary generation failed
    }

    // Split the block
    my_split(bloc, size, canary);

    // Place the canary at the end of the block data in heapdata
    my_place_canary(bloc, canary);

    // Return the address of the data block in heapdata
    my_log_message("RETURN MALLOC: bloc %p bloc->addr %p bloc->size %zu\n", bloc, bloc->addr, bloc->size);
    return bloc->addr;
}

/**
 * @brief Verify the canary value of a block.
 *
 * This function verifies the canary value of the specified block.
 *
 * @param item The block to verify.
 * @return int Returns 1 if the canary is valid, -1 otherwise.
 */
int my_verify_canary(struct chunkmetadata *item)
{
    my_log_message("Verifying canary\n");

    // Calculate the expected canary value
    long    expected_canary = item->canary;

    // Locate the canary at the end of the block
    long    *canary = (long*)((size_t)item->addr + item->size);

    // Verify if the canary matches the expected value
    if (*canary != expected_canary)
    {
        my_log_message("Error: Canary verification failed: Expected %ld but found %ld\n", expected_canary, *canary);
        return -1; // Canary verification failed
    }

    my_log_message("Canary %ld verified\n", *canary);
    return 1; // Canary verification successful
}

/**
 * @brief Clean the memory of a block.
 *
 * This function cleans the memory of the specified block by setting it to zero.
 *
 * @param item The block to clean.
 */
void my_clean_memory(struct chunkmetadata *item)
{
    my_log_message("Cleaning memory at %p of size %zu bytes\n", item->addr, item->size);

    // Set the block's memory to zero
    memset(item->addr, 0, item->size + sizeof(long));

    my_log_message("Memory cleaned\n");
}

/**
 * @brief Merge consecutive free chunks.
 *
 * This function merges consecutive free chunks in the heap metadata.
 */
void my_merge_chunks()
{
    my_log_message("Call merge chunks\n");

    // Iterate over the heapmetadata to merge free chunks
    struct chunkmetadata    *item = heapmetadata;
    while (item != NULL)
    {
        // If the chunk is free, attempt to merge it with the next free chunks
        if (item->flags == FREE)
        {
            struct chunkmetadata    *end = item->next;
            size_t                  new_size = item->size;
            int                     count = 0;

            // Merge consecutive free chunks
            while (end != NULL && end->flags == FREE)
            {
                struct chunkmetadata *next = end;
                my_log_message("Merging chunk at %p with next chunk at %p\n", item->addr, next->addr);
                if (end->next != NULL)
                {
                    new_size += next->size + sizeof(long); // add the size of the canary
                }
                else
                {
                    new_size += next->size;
                }
                count++;
                end = next->next;
                item->size = new_size;
                item->next = end;
            }

            // Update the size of the merged chunk
            // Log the number of chunks merged
            if (count > 0)
            {
                my_log_message("%d chunks merged\n", count);
            }
        }
        item = item->next;
    }

    my_log_message("return merge chunk\n");
    return;
}

/**
 * @brief Free a block of memory.
 *
 * This function frees the specified block of memory.
 *
 * @param ptr A pointer to the memory block to free.
 */
void my_free(void *ptr)
{
    my_log_message("\n\nCALL FREE PTR %p\n", ptr);

    // Check if the heaps is initialized
    if (heapdata == NULL || heapmetadata == NULL)
    {
        my_log_message("Error: Heap not initialized\n");
        return;
    }

    // If ptr is NULL, log an error and return
    if (ptr == NULL)
    {
        my_log_message("Error: Invalid pointer to free: NULL\n");
        return;
    }

    // Verify if ptr is one of the addresses where we allocated memory
    for (struct chunkmetadata *item = heapmetadata; item != NULL; item = item->next)
    {
        // If ptr matches an allocated address in heapmetadata, proceed to free it
        if (item->addr == ptr)
        {
            my_log_message("Found metadata block %p corresponding to ptr %p\n", item, ptr);

            // If the chunk is already free, log an error and return
            if (item->flags == FREE)
            {
                my_log_message("Error: Double free\n");
                return;
            }

            // If the canary is not the one we expect we log an error
            if (my_verify_canary(item) == -1)
            {
                my_log_message("Error: Canary verification failed : Buffer overflow detected\n");
            }

            // Clean the memory before marking it as free
            my_clean_memory(item);

            // Mark the chunk as free
            item->flags = FREE;

            // Merge consecutive free chunks
            my_merge_chunks();

            // Log the event

            my_log_message("RETURN FREE\n");
            return;
        }
    }

    // If ptr is not found in the heap, log an error
    my_log_message("Error: Invalid pointer to free: not in the heap\n");
    return;
}

/**
 * @brief Allocate and zero-initialize an array.
 *
 * This function allocates and zero-initializes an array of the specified size.
 *
 * @param nmemb The number of elements.
 * @param size The size of each element.
 * @return void* A pointer to the allocated and zero-initialized memory, or NULL if the allocation fails.
 */
void* my_calloc(size_t nmemb, size_t size)
{
    my_log_message("\n\nCALL CALLOC nmemb %zu, size %zu\n", nmemb, size);

    // Check if the heap data is initialized
    if (heapdata == NULL)
    {
        my_init_heapdata();
    }

    // Check if the heap metadata is initialized
    if (heapmetadata == NULL)
    {
        my_init_heapmetadata();
    }

    // If the number of elements or size is zero, return NULL
    if (nmemb == 0 || size == 0)
    {
        return NULL;
    }

    // Calculate the total size for allocation
    size_t    total_size = nmemb * size;

    // Allocate memory
    void    *ptr = my_malloc(total_size);

    // If allocation failed, return NULL
    if (ptr == NULL)
    {
        return NULL;
    }

    // Zero-initialize the allocated memory
    memset(ptr, 0, total_size);

    // Return the pointer to the allocated and zero-initialized memory
    my_log_message("RETURN CALLOC : %p\n", ptr);
    return ptr;
}

/**
 * @brief Reallocate memory.
 *
 * This function reallocates the specified block of memory to the new size.
 *
 * @param ptr A pointer to the memory block to reallocate.
 * @param size The new size of the memory block.
 * @return void* A pointer to the reallocated memory, or NULL if the reallocation fails.
 */

void* my_realloc(void *ptr, size_t size)
{
    my_log_message("\n\nCALL REALLOC ptr %p, size %zu\n", ptr, size);

    // Check if the heap data is initialized
    if (heapdata == NULL)
    {
        my_init_heapdata();
    }

    // Check if the heap metadata is initialized
    if (heapmetadata == NULL)
    {
        my_init_heapmetadata();
    }

    // Malloc equivalent
    if (ptr == NULL)
    {
        return my_malloc(size);
    }

    // Free equivalent
    if (size == 0)
    {
        my_free(ptr);
        return  NULL;
    }
	

    for (struct chunkmetadata *item = heapmetadata; item != NULL; item = item->next)
    {
        if (item->addr == ptr)
        {
			my_log_message("Found metadata block %p corresponding to ptr %p\n", item, ptr);
            // Buffer overflow
            if (my_verify_canary(item) == -1)
            {
                my_log_message("Error: Canary verification failed : Buffer overflow detected\n");
            }

			if (size == item->size)
			{
				my_log_message("RETURN REALLOC : %p\n", ptr);	
				return ptr;
			}

            // Locate the canary at the end of the block
            long    canary = *(long*)((size_t)item->addr + item->size);

            // if < size realloc with taille plus petite
            if (size < item->size)
            {
                // Split l'item
                my_split(item, size, canary);
				
                // Place the canary at the end of the block data in heapdata
                my_place_canary(item, canary);
				
				my_merge_chunks();

				my_log_message("RETURN REALLOC : %p\n", item->addr);
                return item->addr;
            }

            // if > size realloc with taille plus grande
            if (size > item->size)
            {
                // If free
                if (item->next->flags == FREE)
                {
                    if (size < item->size + item->next->size)
                    {

						// set metadata for the next item
						item->next->addr = (void*)((size_t)item->addr + size + sizeof(long)); 
                        item->next->size = item->next->size + item->size - size;

						// Set metadata for the new item
                        item->size = size;

                        // Place the canary at the end of the block data in heapdata
                        my_place_canary(item, canary);

						my_log_message("RETURN REALLOC : %p\n", item->addr);
                        return item->addr;
                    }
                }
            }

            void *new_ptr = my_malloc(size);

            if (new_ptr == NULL)
            {
                return NULL;
            }

            memcpy(new_ptr, ptr, size);
            my_free(ptr);

            my_log_message("RETURN REALLOC : %p\n", new_ptr);;
            return new_ptr;
        }
    }
    my_log_message("Error : invalid pointer to realloc : not in the heap\n");
    return NULL;
}


#if DYNAMIC

void *malloc(size_t size)
{
    void    *ptr = my_malloc(size);
    return ptr;
}

void free(void *ptr)
{
    my_free(ptr);
}

void *calloc(size_t nmemb, size_t size)
{
    void    *ptr = my_calloc(nmemb, size);
    return ptr;
}

void *realloc(void *ptr, size_t size)
{
    void    *new_ptr = my_realloc(ptr, size);
    return new_ptr;
}

#endif
