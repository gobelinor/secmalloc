#include "secmalloc.private.h"
#define _GNU_SOURCE
#include <fcntl.h>
#include <stdio.h>
#include <sys/mman.h>
#include <linux/mman.h>
#include <unistd.h>
#include <string.h>
#include "log.h"

// Global variables
size_t heap_size = 4096; // Initial heap size
void *heapdata = NULL;
struct chunkmetadata *heapmetadata = NULL;
size_t heapdata_size = 4096; // Initial data heap size
size_t heapmetadata_size = 4096; // Initial metadata heap size

// Function to initialize heap data
void *init_heapdata()
{
    if (heapdata == NULL)
    {
        heapdata = mmap((void*)(4096*100000), heap_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (heapdata == MAP_FAILED) {
            perror("mmap");
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
        heapmetadata = (struct chunkmetadata*) mmap((void*)(4096*1000), heap_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (heapmetadata == MAP_FAILED) {
            perror("mmap");
            return NULL;
        }
        heapmetadata->size = heap_size - sizeof(struct chunkmetadata);
        heapmetadata->flags = FREE;
        heapmetadata->addr = heapdata;
        heapmetadata->canary = 0xdeadbeef; // Placeholder canary value
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
    if (fd == -1)
    {
        perror("open");
        return -1;
    }
    if (read(fd, &canary, sizeof(long)) == -1)
    {
        perror("read");
        close(fd);
        return -1;
    }
    close(fd);
    return canary;
}

// Function to get the total size of the heap metadata
size_t get_heapmetadata_size()
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
size_t get_heapdata_size()
{
    size_t size = 0;
    for (struct chunkmetadata *item = heapmetadata;
            item != NULL;
            item = item->next)
    {
        size += item->size;
    }
    return size;
}

// Function to resize the heap metadata
void resizeheapmetadata()
{
    void *new_heapmetadata = mremap(heapmetadata, heapmetadata_size, heapmetadata_size + heap_size, MREMAP_MAYMOVE);
    if (new_heapmetadata == MAP_FAILED) {
        perror("mremap");
        return;
    }
    heapmetadata = new_heapmetadata;
    heapmetadata_size += heap_size;
}

// Function to resize the heap data
void resizeheapdata()
{
    void *new_heapmetadata = mremap(heapmetadata, heapmetadata_size, heapmetadata_size + heap_size, MREMAP_MAYMOVE);
    if (new_heapmetadata == MAP_FAILED) {
        perror("mremap");
        return;
    }
    heapmetadata = new_heapmetadata;
    heapmetadata_size += heap_size;
}

// Function to look up a free block with enough size
struct chunkmetadata *lookup(size_t size)
{
    for (struct chunkmetadata *item = heapmetadata;
            item != NULL;
            item = item->next)
    {
        if (item->flags == FREE && item->size >= size)
        {
            return item;
        }
    }
    return NULL;
}

// Function to get the last metadata block
struct chunkmetadata *lastmetadata()
{
    struct chunkmetadata *item = heapmetadata;
    while (item->next != NULL)
    {
        item = item->next;
    }
    return item;
}

// Function to split a block into two, returning the new second block
void split(struct chunkmetadata *bloc, size_t size)
{
    // Create new metadata block for the second part
    struct chunkmetadata *newbloc = lastmetadata();
    // Set metadata for new block
    newbloc->size = bloc->size - size;
    newbloc->flags = FREE;
    newbloc->addr = (void*)((size_t)bloc->addr + size);
    newbloc->canary = 0xdeadbeef; // Placeholder canary
    newbloc->next = bloc->next;
    newbloc->prev = bloc;
    // Update first block metadata
    bloc->size = size;
    bloc->next = newbloc;
    return;
}

// Function to place a canary at the end of a block
void place_canary(struct chunkmetadata *bloc, long canary)
{
    long *canary_ptr = (long*)((size_t)bloc->addr + bloc->size);
    *canary_ptr = canary;
}

// Function to allocate memory of the specified size
void* my_malloc(size_t size)
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

    // Get the total size of metadata heap and resize it if needed
    size_t current_heapmetadata_size = get_heapmetadata_size();
    if (4096 - current_heapmetadata_size % 4096 < sizeof(struct chunkmetadata))
    {
        resizeheapmetadata();
    }

    // Get the total size of data heap and resize it if needed
    size_t current_heapdata_size = get_heapdata_size();
    if (4096 - current_heapdata_size % 4096 < size)
    {
        resizeheapdata();
    }

    // Get metadata block of free data block with large enough size
    struct chunkmetadata *bloc = lookup(size);
    if (bloc == NULL)
    {
        return NULL; // No suitable block found
    }

    // Generate a canary
    long canary = generate_canary();
    if (canary == -1) {
        return NULL; // Canary generation failed
    }

    // Split the block
    split(bloc, size);

    // Fill the block with metadata
    bloc->flags = BUSY;
    bloc->canary = canary;

    // Place the canary at the end of the block data in heapdata
    place_canary(bloc, canary);

    // Return the address of the data block in heapdata
    return bloc->addr;
}

// Function to check if a pointer is valid (not used, included in free)
int is_valid(void *ptr)
{
    // Check if ptr is in heapdata
    for (struct chunkmetadata *item = heapmetadata;
            item != NULL;
            item = item->next)
    {
        if (item->addr == ptr)
        {
            return 1;
        }
    }
    return 0;
}

// Function to verify the canary value of a block
int verify_canary(struct chunkmetadata *item)
{
    log_message("Verifying canary");
    long expected_canary = item->canary;
    long *canary = (long*)((size_t)item->addr + item->size);
    if (*canary != expected_canary)
    {
        return -1; // Canary verification failed
    }
    log_message("Canary verified");
    return 1;
}

// Function to clean the memory of a block
void clean_memory(struct chunkmetadata *item)
{
    log_message("Cleaning memory");
    memset(item->addr, 0, item->size);
    log_message("Memory cleaned");
}

// Function to merge consecutive free chunks
void merge_chunks()
{
    log_message("Merging chunks");
    for (struct chunkmetadata *item = heapmetadata;
            item != NULL;
            item = item->next)
    {
        if (item->flags == FREE)
        {
            struct chunkmetadata *end = item;
            size_t new_size = item->size;
            int count = 0;

            while (end->flags == FREE && end->next != NULL)
            {
                end = end->next;
                if (end->flags == FREE)
                {
                    new_size += end->size;
                    count++;
                }
            }

            item->size = new_size;
            item->next = end->next;
            if (end->next != NULL)
            {
                end->next->prev = item;
            }
            if (count > 0)
            {
                log_message("%d chunks merged", count);
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
        log_message("Heap not initialized");
        return;
    }

    // If ptr is NULL, log an error
    if (ptr == NULL)
    {
        log_message("Invalid pointer to free : NULL");
        return;
    }

    // Verify if ptr is one of the addresses where memory was allocated
    for (struct chunkmetadata *item = heapmetadata;
            item != NULL;
            item = item->next)
    {
        if (item->addr == ptr)
        {
            // If the chunk is already free, log an error
            if (item->flags == FREE)
            {
                log_message("Double free");
                return;
            }

            // Verify the canary value
            if (verify_canary(item) == -1)
            {
                log_message("Canary verification failed : Buffer overflow detected");
                return;
            }

            // Clean the memory before freeing it
            clean_memory(item);

            // Free the chunk
            item->flags = FREE;

            // Log the event
            log_event(FREE_fn, END, ptr, item->size);

            // Merge free chunks
            merge_chunks();
            return;
        }
    }

    // If ptr is not in the heap, log an error
    log_message("Invalid pointer to free : not in the heap");
    return;
}