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
void *heapdata = NULL; // Pointer to the heap data
struct chunkmetadata *heapmetadata = NULL; // Pointer to the heap metadata
size_t pageheap_size = 4096; // Constant size for page heap
size_t heapdata_size = 4096; // Current size of the heap data, will increase as needed
size_t heapmetadata_size = 4096; // Current size of the heap metadata, will increase as needed
size_t max_metadata_size = 100000 * sizeof(struct chunkmetadata); // Maximum size of the heap metadata
void* base_address = (void*)(4096 * 1000); // Base address for memory mapping

// Function to initialize heap data
void *init_heapdata()
{
    if (heapdata == NULL)
    {
        // Attempt to map memory for heap data
		heapdata = mmap((void*)((size_t)base_address+max_metadata_size), pageheap_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

        // Check if the mmap operation was successful
        if (heapdata == MAP_FAILED) {
            perror("mmap");
            log_message("Error: Failed to mmap memory for heap data.");
            return NULL;
        }
    }
    return heapdata;
}

// Function to initialize heap metadata
struct chunkmetadata *init_heapmetadata()
{
	if (heapmetadata == NULL)
	{
	    // Attempt to map memory for heap metadata
		heapmetadata = (struct chunkmetadata*) mmap(base_address, pageheap_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

        // Check if the mmap operation was successful
        if (heapmetadata == MAP_FAILED) {
            perror("mmap");
            log_message("Error: Failed to mmap memory for heap metadata.");
            return NULL;
        }

        // Initialize the first chunk metadata
		heapmetadata->size = pageheap_size;
		heapmetadata->flags = FREE;
		heapmetadata->addr = heapdata;
		heapmetadata->canary = 0xdeadbeef; // will be replaced by a random value during first malloc
		heapmetadata->next = NULL;
		heapmetadata->prev = NULL;
	}
	return heapmetadata;
}

// Function to generate a random canary value
long generate_canary()
{
    long canary = 0;
    int fd = open("/dev/urandom", O_RDONLY);

    // Check if opening /dev/urandom was successful
    if (fd == -1)
    {
        perror("open");
        log_message("Error: Failed to open /dev/urandom for canary generation.");
        return -1;
    }

    // Read random data into the canary variable
    ssize_t result = read(fd, &canary, sizeof(long));
    if (result == -1)
    {
        perror("read");
        log_message("Error: Failed to read from /dev/urandom for canary generation.");
        close(fd); // Close the file descriptor before returning
        return -1;
    }
    else if (result != sizeof(long))
    {
        log_message("Error: Incomplete read from /dev/urandom. Expected %zu bytes but got %zd bytes.", sizeof(long), result);
        close(fd); // Close the file descriptor before returning
        return -1;
    }

    close(fd); // Close the file descriptor after reading
    return canary;
}


// Function to get the total size of the heap metadata
size_t get_allocated_heapmetadata_size()
{
	size_t size = 0;
	for (struct chunkmetadata *item = heapmetadata;
			item != NULL;
			item = item->next)
	{
		size += sizeof(struct chunkmetadata);
	}
	return size;
}

// Function to get the total size of the heap data
size_t get_allocated_heapdata_size()
{
	struct chunkmetadata *last_item = NULL;
	size_t size = 0;
	for (struct chunkmetadata *item = heapmetadata;
			item != NULL;
			item = item->next)
	{
		size += item->size + sizeof(long); // add the size of the canary
		last_item = item;
	}
	if (last_item->flags == FREE)
	{
		size -= last_item->size;
	}
	return size;
}

// Function to get the last metadata bloc
struct chunkmetadata *lastmetadata()
{
	struct chunkmetadata *item = heapmetadata;
	while (item->next != NULL)
	{
		item = item->next;
	}
	return item;
}

// Function to resize the heap metadata
void resizeheapmetadata()
{
    // Ensure the current heap metadata size is valid
    if (heapmetadata == NULL) {
        log_message("Heap metadata is not initialized.");
        return;
    }

    // Calculate the new size of the heap metadata
    size_t new_size = heapmetadata_size + pageheap_size;

    // Attempt to resize the heap metadata using mremap
    void *new_heapmetadata = mremap(heapmetadata, heapmetadata_size, new_size, MREMAP_MAYMOVE);

    // Check if the remapping was successful
    if (new_heapmetadata == MAP_FAILED) {
        perror("mremap");
        log_message("Failed to resize heap metadata.");
        return;
    }

    // Update the heap metadata pointer and size
    heapmetadata = new_heapmetadata;
    heapmetadata_size = new_size;
}

// Function to resize the heap data
void resizeheapdata(size_t new_size)
{
    // Ensure the current heap data size is valid
    if (heapdata == NULL) {
        log_message("Heap data is not initialized.");
        return;
    }

    // Attempt to resize the heap data using mremap
    void *new_heapdata = mremap(heapdata, heapdata_size, new_size, MREMAP_MAYMOVE);

    // Check if the remapping was successful
    if (new_heapdata == MAP_FAILED) {
        perror("mremap");
        log_message("Failed to resize heap data.");
        return;
    }

    // Update the heap data pointer and size
    heapdata = new_heapdata;
    heapdata_size = new_size;

    // Get the last metadata block
    struct chunkmetadata *last = lastmetadata();
    if (last != NULL) {
        last->size = new_size;
    } else {
        log_message("No metadata found to update the size.");
    }
}

// Function to look up a free block with enough size
struct chunkmetadata *lookup(size_t size)
{
    // Check if the heap metadata is initialized
    if (heapmetadata == NULL) {
        log_message("Heap metadata is not initialized.");
        return NULL;
    }

    // Traverse the linked list of chunkmetadata to find a suitable free block
    for (struct chunkmetadata *item = heapmetadata; item != NULL; item = item->next)
    {
        // Check if the current block is free and has enough size
        if (item->flags == FREE && item->size >= size + sizeof(long))
        {
            log_message("Found suitable free block of size %zu bytes.", item->size);
            return item; // Return the suitable free block
        }
    }

    log_message("No suitable free block found for size %zu bytes.", size);
    return NULL; // Return NULL if no suitable block is found
}

// Function to split a block into two, returning the new second block
void split(struct chunkmetadata *bloc, size_t size)
{
    // Check if the block to be split is valid
    if (bloc == NULL) {
        log_message("Error: Attempted to split a NULL block.");
        return;
    }

	//create new metadata bloc for the second part
	struct chunkmetadata *newbloc = (struct chunkmetadata*) ((size_t)lastmetadata()+sizeof(struct chunkmetadata));

    // Check if the new block address is within the heap
    if ((size_t)newbloc >= (size_t)heapmetadata + heapmetadata_size) {
        log_message("Error: New block address %p is out of heap bounds.", newbloc);
        return;
    }

    // Set metadata for the new block
	newbloc->size = bloc->size - size - sizeof(long);
	newbloc->flags = FREE;
	newbloc->addr = (void*)((size_t)bloc->addr + size + sizeof(long));
	newbloc->canary = 0xdeadbeef;
	newbloc->next = bloc->next;
	newbloc->prev = bloc;

	// Update first bloc metadata
	bloc->size = size;
	bloc->next = newbloc;

    // Update the previous pointer of the next block if it exists
    if (newbloc->next != NULL) {
        newbloc->next->prev = newbloc;
    }

	return;
}

void place_canary(struct chunkmetadata *bloc, long canary)
{
	// Calculate the address where the canary should be placed
	long *canary_ptr = (long*)((size_t)bloc->addr + bloc->size);

    // Place the canary value at the calculated address
	*canary_ptr = canary;
}

// Function to allocate memory of the specified size
void* my_malloc(size_t size)
{
	log_event(MALLOC, START, NULL, size);
    // If requested size is 0, return NULL
    if (size == 0)
    {
        return NULL;
    }

    // Check if the heap is initialized
    if (heapdata == NULL)
    {
        if (init_heapdata() == NULL)
        {
            return NULL; // Initialization failed
        }
    }

    // Check if the heap metadata is initialized
    if (heapmetadata == NULL)
    {
        if (init_heapmetadata() == NULL)
        {
            return NULL; // Initialization failed
        }
    }

    // Get the total size of allocated metadata heap and resize if needed
    size_t allocated_heapmetadata_size = get_allocated_heapmetadata_size();
    if (4096 - allocated_heapmetadata_size % 4096 < sizeof(struct chunkmetadata))
    {
    	log_message("resizeheapmetadata\n");
        resizeheapmetadata();
    }

    // Get the total size of allocated data heap and resize if needed
    size_t allocated_heapdata_size = get_allocated_heapdata_size();
    size_t needed_size = size + sizeof(long);
    size_t available_size = heapdata_size - allocated_heapdata_size;
    if (available_size < needed_size)
    {
        size_t new_size = allocated_heapdata_size + needed_size;
        new_size = ((new_size / 4096) + ((new_size % 4096 != 0) ? 1 : 0)) * 4096;
        log_message("resizeheapdata : newsize %ld\n", new_size);
        resizeheapdata(new_size);
    }

    // Look up a free block with large enough size
    struct chunkmetadata *bloc = lookup(size);
    if (bloc == NULL)
    {
        return NULL; // No suitable block found
    }

    // Generate a canary
    long canary = generate_canary();
    if (canary == -1)
    {
        return NULL; // Canary generation failed
    }

    // Split the block
    split(bloc, size);

    // Fill the block with metadata
    bloc->flags = BUSY;
    bloc->canary = canary;

    // Place the canary at the end of the block data in heapdata
    place_canary(bloc, canary);

	log_event(MALLOC, END, bloc->addr, size);

    // Return the address of the data block in heapdata
    return bloc->addr;
}

// Function to verify the canary value of a block
int verify_canary(struct chunkmetadata *item)
{
    log_message("Verifying canary\n");

    // Calculate the expected canary value
    long expected_canary = item->canary;

    // Locate the canary at the end of the block
    long *canary = (long*)((size_t)item->addr + item->size);

    // Verify if the canary matches the expected value
    if (*canary != expected_canary)
    {
        log_message("Canary verification failed: Expected %ld but found %ld\n", expected_canary, *canary);
        return -1; // Canary verification failed
    }

    log_message("Canary verified\n");
    return 1; // Canary verification successful
}

// Function to clean the memory of a block
void clean_memory(struct chunkmetadata *item)
{
    log_message("Cleaning memory\n");

    // Set the block's memory to zero
    memset(item->addr, 0, item->size);

    log_message("Memory cleaned\n");
}

// Function to merge consecutive free chunks
void merge_chunks()
{
    log_message("Merging chunks\n");

    // Iterate over the heapmetadata to merge free chunks
    for (struct chunkmetadata *item = heapmetadata; item != NULL; item = item->next)
    {
        // If the chunk is free, attempt to merge it with the next free chunks
        if (item->flags == FREE)
        {
            struct chunkmetadata *end = item;
            size_t new_size = item->size;
            int count = 0;

            // Merge consecutive free chunks
            while (end->flags == FREE && end->next != NULL)
            {
                end = end->next;
                if (end->flags == FREE)
                {
                    new_size += end->size + sizeof(struct chunkmetadata);
                    count++;
                    item->size = new_size;
                    item->next = end->next;
                }
            }

            // Update the previous pointer of the next chunk
            if (end->next != NULL)
            {
                end->next->prev = item;
            }

            // Log the number of chunks merged
            if (count > 0)
            {
                log_message("%d chunks merged\n", count);
            }
        }
    }
}

// Function to free a block of memory
void my_free(void *ptr)
{
    log_event(FREE_fn, START, ptr, 0);

    // Check if the heap is initialized
    if (heapdata == NULL || heapmetadata == NULL)
    {
        log_message("Heap not initialized\n");
        return;
    }

    // If ptr is NULL, log an error and return
    if (ptr == NULL)
    {
        log_message("Invalid pointer to free: NULL\n");
        return;
    }

    // Verify if ptr is one of the addresses where we allocated memory
    for (struct chunkmetadata *item = heapmetadata; item != NULL; item = item->next)
    {
        // If ptr matches an allocated address in heapmetadata, proceed to free it
        if (item->addr == ptr)
		{
            // If the chunk is already free, log an error and return
			if (item->flags == FREE)
			{
				log_message("Double free\n");
				return;
			}

			// If the canary is not the one we expect we log an error
			if (verify_canary(item) == -1)
			{
				log_message("Canary verification failed : Buffer overflow detected\n");
			}

            // Clean the memory before marking it as free
            clean_memory(item);

            // Mark the chunk as free
            item->flags = FREE;

            // Log the event
            log_event(FREE_fn, END, ptr, item->size);

            // Merge consecutive free chunks
            merge_chunks();
            return;
		}
	}

    // If ptr is not found in the heap, log an error
    log_message("Invalid pointer to free: not in the heap\n");
	return;
}

// Function to allocate and zero-initialize array
void* my_calloc(size_t nmemb, size_t size)
{
    // Check if the heap is initialized
    if (heapdata == NULL)
    {
        init_heapdata();
    }

    // Check if the heap metadata is initialized
    if (heapmetadata == NULL)
    {
        init_heapmetadata();
    }

    // If the number of elements or size is zero, return NULL
    if (nmemb == 0 || size == 0)
    {
        return NULL;
    }

    // Calculate the total size for allocation
    size_t total_size = nmemb * size;

    // Allocate memory
    void *ptr = my_malloc(total_size);

    // If allocation failed, return NULL
    if (ptr == NULL)
    {
        return NULL;
    }

    // Set the allocated memory to zero
    memset(ptr, 0, total_size);

    // Return the pointer to the allocated and zero-initialized memory
    return ptr;
}

// Function to reallocate memory
void *my_realloc(void *ptr, size_t size)
{
    // Check if the heap is initialized
    if (heapdata == NULL)
    {
        init_heapdata();
    }

    // Check if the heap metadata is initialized
    if (heapmetadata == NULL)
    {
        init_heapmetadata();
    }

    // If ptr is NULL, equivalent to malloc
    if (ptr == NULL)
    {
        return my_malloc(size);
    }

    // If size is 0, equivalent to free
    if (size == 0)
    {
        my_free(ptr);
        return NULL;
    }

    // Find the metadata block corresponding to ptr
    for (struct chunkmetadata *item = heapmetadata; item != NULL; item = item->next)
    {
        if (item->addr == ptr)
        {
            // Verify the canary value to detect buffer overflows
            if (verify_canary(item) == -1)
            {
                log_message("Realloc: Canary verification failed: Buffer overflow detected\n");
            }

            // Allocate new memory of the specified size
            void *new_ptr = my_malloc(size);

            // If allocation failed, return NULL
            if (new_ptr == NULL)
            {
                return NULL;
            }

            // Copy data from the old memory to the new memory
            size_t copy_size = (item->size < size) ? item->size : size;
            memcpy(new_ptr, ptr, copy_size);

            // Free the old memory
            my_free(ptr);

            // Return the new pointer
            return new_ptr;
        }
    }

    // If ptr is not in the heap, log an error
    log_message("Invalid pointer to realloc: not in the heap\n");
    return NULL;
}

#if DYNAMIC

void *malloc(size_t size) 
{
    void *ptr = my_malloc(size);
    return ptr;
}

void free(void *ptr) 
{
    my_free(ptr);
}

void *calloc(size_t nmemb, size_t size) 
{
    void *ptr = my_calloc(nmemb, size);
    return ptr;
}

void *realloc(void *ptr, size_t size) 
{
    void *new_ptr = my_realloc(ptr, size);
    return new_ptr;
}

#endif
