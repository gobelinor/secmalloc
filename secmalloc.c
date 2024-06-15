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
size_t heapdata_size = PAGE_HEAP_SIZE; // Current size of the heap data, will increase as needed
size_t heapmetadata_size = PAGE_HEAP_SIZE; // Current size of the heap metadata, will increase as needed

// Function to initialize heap data
void *my_init_heapdata()
{
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
    return heapdata;
}

// Function to initialize heap metadata
struct chunkmetadata *my_init_heapmetadata()
{
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
	return heapmetadata;
}

// Function to generate a random canary value
long my_generate_canary()
{
    long canary = 0;
    int fd = open("/dev/urandom", O_RDONLY);

    // Check if opening /dev/urandom was successful
    if (fd == -1)
    {
        perror("open");
        my_log_message("Error: Failed to open /dev/urandom for canary generation.\n");
        return -1;
    }

    // Read random data into the canary variable
    ssize_t result = read(fd, &canary, sizeof(long));
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
    return canary;
}


// Function to get the total alocated size of the heap metadata
size_t my_get_allocated_heapmetadata_size()
{
	size_t size = 0;
	/* for (struct chunkmetadata *item = heapmetadata; */
	/* 		item != NULL; */
	/* 		item = item->next) */
	/* { */
	/*     log_message("Traversing metadata block at %p with next at %p\n", item, item->next); */
	/* 	size += sizeof(struct chunkmetadata); */
	/* } */
	void *item = heapmetadata;
	while (((struct chunkmetadata*)item)->size != 0)	
	{
		size += sizeof(struct chunkmetadata);
		item = (void*)((size_t)item + sizeof(struct chunkmetadata));
	}
	return size;
}

// Function to get the total alocated size of the heap data
size_t my_get_allocated_heapdata_size()
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
struct chunkmetadata *my_lastmetadata()
{
	struct chunkmetadata *item = heapmetadata;
	while (item->next != NULL)
	{
		item = item->next;
	}
	my_log_message("Last metadata block at %p, size : %zu, flags : %d\n", item, item->size, item->flags);
	return item;
}

// Function to resize the heap metadata
void my_resizeheapmetadata()
{
    // Ensure the current heap metadata size is valid
    if (heapmetadata == NULL) {
        my_log_message("Heap metadata is not initialized.\n");
        return;
    }

    // Calculate the new size of the heap metadata
    size_t new_size = heapmetadata_size + PAGE_HEAP_SIZE;

    // Attempt to resize the heap metadata using mremap
    void *new_heapmetadata = mremap(heapmetadata, heapmetadata_size, new_size, MREMAP_MAYMOVE);

    // Check if the remapping was successful
    if (new_heapmetadata == MAP_FAILED) {
        perror("mremap");
        my_log_message("Failed to resize heap metadata.\n");
        return;
    }

    // Update the heap metadata pointer and size
    heapmetadata = new_heapmetadata;
    heapmetadata_size = new_size;
}

// Function to resize the heap data
void my_resizeheapdata(size_t new_size)
{
    // Ensure the current heap data size is valid
    if (heapdata == NULL) {
        my_log_message("Heap data is not initialized.\n");
        return;
    }

	void *old_heapdata = heapdata;
    // Attempt to resize the heap data using mremap
    void *new_heapdata = mremap(heapdata, heapdata_size, new_size, MREMAP_MAYMOVE);
	
	if (old_heapdata != new_heapdata){
		my_log_message("old_heapdata != new_heapdata\n");
		my_log_message("old_heapdata : %p\n", old_heapdata);
		my_log_message("new_heapdata : %p\n", new_heapdata);
	}

    // Check if the remapping was successful
    if (new_heapdata == MAP_FAILED) {
        perror("mremap");
        my_log_message("Failed to resize heap data.\n");
        return;
    }

    // Update the heap data pointer and size
    heapdata = new_heapdata;
    heapdata_size = new_size;

    // Get the last metadata block
    struct chunkmetadata *last = my_lastmetadata();
    if (last != NULL) 
	{
        last->size = new_size;
    } 
	else 
	{
        my_log_message("No metadata found to update the size.\n");
    }
}

// Function to look up a free block with enough size
struct chunkmetadata *my_lookup(size_t size)
{
    // Check if the heap metadata is initialized
    if (heapmetadata == NULL) {
        my_log_message("Heap metadata is not initialized.\n");
        return NULL;
    }

    // Traverse the linked list of chunkmetadata to find a suitable free block
    for (struct chunkmetadata *item = heapmetadata; item != NULL; item = item->next)
    {
        // Check if the current block is free and has enough size
        if (item->flags == FREE && item->size >= size + sizeof(long))
        {
            my_log_message("Found suitable free block %p of size %zu bytes.\n", item->addr, item->size);
            return item; // Return the suitable free block
        }
    }

    my_log_message("No suitable free block found for size %zu bytes.\n", size);
    return NULL; // Return NULL if no suitable block is found
}

// Function to split a block into two, returning the new second block
void my_split(struct chunkmetadata *bloc, size_t size, long canary)
{
	
	my_log_message("Splitting block %p at %p of size %zu bytes into %zu bytes and %zu bytes.\n", bloc,  bloc->addr, bloc->size, size, bloc->size - size - sizeof(long));
    // Check if the block to be split is valid
    if (bloc == NULL) {
        my_log_message("Error: Attempted to split a NULL block.\n");
        return;
    }
	my_log_message("bloc->next = %p\n", bloc->next);
	my_log_message("call lastmetadata\n");
	//create new metadata bloc for the second part
	struct chunkmetadata *newbloc = (struct chunkmetadata*) ((size_t)heapmetadata+my_get_allocated_heapmetadata_size());
	my_log_message("in split : new newbloc = %p, size = %zu, flags = %d\n", newbloc, newbloc->size, newbloc->flags);
	// check if the newbloc is not out of the heapmetadata
	if ((size_t)newbloc >= (size_t)heapmetadata + heapmetadata_size)
	{
		my_log_message("Error: Attempted to split a block out of the heapmetadata.\n");
		return;
	}

	//check if the newbloc is not out of the heapdata
	if ((size_t)newbloc->addr >= (size_t)heapdata + heapdata_size)
	{
		my_log_message("Error: Attempted to split a block out of the heapdata.\n");
		return;
	}
	
	/* memset(newbloc, 0, sizeof(struct chunkmetadata)); */
	// Set metadata for the new block
	newbloc->size = bloc->size - size - sizeof(long);
	newbloc->flags = FREE;
	newbloc->addr = (void*)((size_t)bloc->addr + size + sizeof(long));
	newbloc->canary = 0xdeadbeef;
	newbloc->next = bloc->next;
	my_log_message("in split : filled newbloc = %p, size = %zu, flags = %d, addr = %p, canary = %ld, next = %p\n", newbloc, newbloc->size, newbloc->flags, newbloc->addr, newbloc->canary, newbloc->next);
	
	// Set the metadata for the first block	
	// bloc == newbloc should really not happen 
	if (bloc == newbloc)
	{
		my_log_message("CURSED : bloc %p == newbloc %p\n", bloc, newbloc);
		bloc->next = newbloc->next;
	}
	else
	{
		// sould always be the case
		bloc->next = newbloc;
	}
	bloc->size = size;
    bloc->flags = BUSY;
    bloc->canary = canary;
	return;
}

void my_place_canary(struct chunkmetadata *bloc, long canary)
{
	// Calculate the address where the canary should be placed
	long *canary_ptr = (long*)((size_t)bloc->addr + bloc->size);

    // Place the canary value at the calculated address
	*canary_ptr = canary;
}

// Function to allocate memory of the specified size
void* my_malloc(size_t size)
{
	my_log_message("CALL MALLOC SIZE %zu\n", size);
	/* write(2, "my_malloc\n", 10); */
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
	my_log_message("call get_allocated_heapmetadata_size\n");
    size_t allocated_heapmetadata_size = my_get_allocated_heapmetadata_size();
    if (PAGE_HEAP_SIZE - allocated_heapmetadata_size % PAGE_HEAP_SIZE < sizeof(struct chunkmetadata))
    {
    	my_log_message("resizeheapmetadata\n");
        my_resizeheapmetadata();
    }

    // Get the total size of allocated data heap and resize if needed
    size_t allocated_heapdata_size = my_get_allocated_heapdata_size();
    size_t needed_size = size + sizeof(long);
    size_t available_size = heapdata_size - allocated_heapdata_size;
    if (available_size < needed_size)
    {
        size_t new_size = allocated_heapdata_size + needed_size;
        new_size = ((new_size / PAGE_HEAP_SIZE) + ((new_size % PAGE_HEAP_SIZE != 0) ? 1 : 0)) * PAGE_HEAP_SIZE;
        my_log_message("resizeheapdata : newsize %ld\n", new_size);
        my_resizeheapdata(new_size);
    }

    // Look up a free block with large enough size
    struct chunkmetadata *bloc = my_lookup(size);
    if (bloc == NULL)
    {
        return NULL; // No suitable block found
    }

    // Generate a canary
    long canary = my_generate_canary();
    if (canary == -1)
    {
        return NULL; // Canary generation failed
    }

    // Split the block
	my_log_message("call split(bloc : %p, size %ld)\n", bloc->addr, size);
    my_split(bloc, size, canary);

    // Place the canary at the end of the block data in heapdata
    my_place_canary(bloc, canary);

    // Return the address of the data block in heapdata
	my_log_message("RETURN MALLOC: bloc %p bloc->addr %p\n bloc->size %zu\n", bloc, bloc->addr, bloc->size);
    return bloc->addr;
}

// Function to verify the canary value of a block
int my_verify_canary(struct chunkmetadata *item)
{
    my_log_message("Verifying canary\n");

    // Calculate the expected canary value
    long expected_canary = item->canary;

    // Locate the canary at the end of the block
    long *canary = (long*)((size_t)item->addr + item->size);

    // Verify if the canary matches the expected value
    if (*canary != expected_canary)
    {
        my_log_message("Canary verification failed: Expected %ld but found %ld\n", expected_canary, *canary);
        return -1; // Canary verification failed
    }

    my_log_message("Canary verified\n");
    return 1; // Canary verification successful
}

// Function to clean the memory of a block
void my_clean_memory(struct chunkmetadata *item)
{
    my_log_message("Cleaning memory\n");

    // Set the block's memory to zero
    memset(item->addr, 0, item->size + sizeof(long));

    my_log_message("Memory cleaned\n");
}

// Function to merge consecutive free chunks
void my_merge_chunks()
{
	my_log_message("Merging chunks\n");

	// Iterate over the heapmetadata to merge free chunks
	struct chunkmetadata *item = heapmetadata;
	while (item != NULL)
	{
		// If the chunk is free, attempt to merge it with the next free chunks
		if (item->flags == FREE)
		{
			struct chunkmetadata *end = item->next;
			size_t new_size = item->size;
			int count = 0;

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
}

// Function to free a block of memory
void my_free(void *ptr)
{
	my_log_message("CALL FREE PTR %p\n", ptr);

    // Check if the heaps is initialized
    if (heapdata == NULL || heapmetadata == NULL)
    {
        my_log_message("Heap not initialized\n");
        return;
    }

    // If ptr is NULL, log an error and return
    if (ptr == NULL)
    {
        my_log_message("Invalid pointer to free: NULL\n");
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
				my_log_message("Double free\n");
				return;
			}

			// If the canary is not the one we expect we log an error
			if (my_verify_canary(item) == -1)
			{
				my_log_message("Canary verification failed : Buffer overflow detected\n");
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
    my_log_message("Invalid pointer to free: not in the heap\n");
	return;
}

// Function to allocate and zero-initialize array
void* my_calloc(size_t nmemb, size_t size)
{
	my_log_message("CALL CALLOC nmemb %zu, size %zu\n", nmemb, size);
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
    size_t total_size = nmemb * size;

    // Allocate memory
    void *ptr = my_malloc(total_size);

    // If allocation failed, return NULL
    if (ptr == NULL)
    {
        return NULL;
    }

    // Return the pointer to the allocated and zero-initialized memory
	my_log_message("RETURN CALLOC : %p\n", ptr);
    return ptr;
}

// Function to reallocate memory
void *my_realloc(void *ptr, size_t size)
{
	my_log_message("CALL REALLOC PTR %p, SIZE %zu\n", ptr, size);

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
            if (my_verify_canary(item) == -1)
            {
                my_log_message("Realloc: Canary verification failed: Buffer overflow detected\n");
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
			my_log_message("RETURN REALLOC : %p\n", new_ptr);
            return new_ptr;
        }
    }

    // If ptr is not in the heap, log an error
    my_log_message("Invalid pointer to realloc: not in the heap\n");
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
