#include "my_alloc.private.h"
#define _GNU_SOURCE
#include <stdio.h>
#include <sys/mman.h>
#include <linux/mman.h>

// Global variables
struct chunk *heap = NULL; // Pointer to the beginning of the heap
size_t heap_size = 4096;

// Function to initialize the heap
struct chunk *init_heap()
{
	if (heap == NULL)
	{
	    // Allocate memory for the heap using mmap
		heap = (struct chunk*) mmap((void*)(4096*100000), heap_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

        // Initialize the first chunk
        if (heap == MAP_FAILED) {
            return NULL; // Return NULL if mmap failed
        }
		heap->size = heap_size - sizeof(struct chunk);
		heap->flags = FREE;
	}
	return heap;
}

// Function to get the last raw chunk in the heap
struct chunk *get_last_chunk_raw()
{
	for (struct chunk *item = heap;
			(size_t)item < (size_t) heap + heap_size;
			item = (struct chunk *)((size_t)item + sizeof(struct chunk) + item->size)
			)
	{
		printf("last chunk check %p size %lu - flag %u\n", item, item->size, item->flags);
		if ((size_t)item + sizeof(struct chunk) + item->size >= (size_t) heap + heap_size)
		{
			printf("ret %p\n", item);
			return item;
		}
		printf("skip\n");
	}
	return NULL;
}

// Function to get a free chunk without any additional checks
struct chunk *get_free_chunk_raw(size_t size)
{
    if (heap == NULL) {
        heap = init_heap();
        if (heap == NULL) {
            return NULL; // Return NULL if heap initialization failed
        }
    }

	for (struct chunk *item = heap;
			(size_t)item < (size_t)heap + heap_size;
			item = (struct chunk *)((size_t)item + item->size + sizeof(struct chunk))
			)
	{
		if (item->flags == FREE && item->size >= size)
			return item;
	}
	return NULL;
}

// Function to get a free chunk of the specified size
struct chunk *get_free_chunk(size_t size)
{
    if (heap == NULL) {
        heap = init_heap();
        if (heap == NULL) {
            return NULL; // Return NULL if heap initialization failed
        }
    }

	printf("heap %p\n", heap);
	struct chunk *item = get_free_chunk_raw(size);

	if (item == NULL)
	{
        // Not enough memory space, need to remap
		printf("HERE %p\n", item);
		size_t tot_size = size + sizeof(struct chunk);
		size_t old_size = heap_size;
		size_t delta_size = ((tot_size/4096) + ((tot_size % 4096 != 0) ? 1 : 0)) * 4096;
		struct chunk *last_item = get_last_chunk_raw();
		heap_size += delta_size;
		printf("HEAP NEW SIZE %lu\n", heap_size);

		struct chunk *new_heap = mremap(heap, old_size, heap_size, MREMAP_MAYMOVE);
		printf("HEAP resized %p\n", new_heap);

		if (new_heap != heap)
            return NULL; // Verify that the heap hasn't been moved

		printf("LAST SIZE %lu - %p\n", delta_size, last_item);
		last_item->size += delta_size;
		printf("last chunk %p size %lu - flag %u\n", last_item, last_item->size, last_item->flags);
		item = get_free_chunk_raw(size);
		printf("item chunk %p\n", item);
	}
	return item;
}

// Function to look up a chunk of the specified size
struct chunk *lookup(size_t size) {
    if (heap == NULL) {
        heap = init_heap();  // Initialize heap if not already initialized
        if (heap == NULL) {
            return NULL; // Return NULL if heap initialization failed
        }
    }

    struct chunk *current = heap;
    while ((size_t)current < (size_t)heap + heap_size) {
        if (current->flags == FREE && current->size >= size) {
            return current;  // Found a free block with enough space
        }
        // Move to the next chunk in the heap memory
        current = (struct chunk *)((size_t)current + sizeof(struct chunk) + current->size);
    }

    // No suitable block found, return NULL
    return NULL;
}

// Function to allocate memory of the specified size
void *my_alloc(size_t size) {
	(void) size;
	void *ptr;

    // Get a free chunk of the required size
	struct chunk *ch = get_free_chunk(size);

    if (ch == NULL) {
        return NULL; // Allocation failed
    }

    // Split the chunk
	ptr =(void*) ((size_t)ch + sizeof(struct chunk));

    // Create a new chunk at the end of the allocated memory
	struct chunk *end = (struct chunk*)((size_t)ptr + size);
	end->flags = FREE;
	end->size = ch->size - sizeof(struct chunk) - size;
	ch->flags = BUSY;
	ch->size = size;
	return ptr;
}

// Function to clean and free a specified chunk
void clean(void *ptr)
{
    if (ptr == NULL) {
        return; // Return immediately if the pointer is NULL
    }

	struct chunk *ch = (struct chunk*)((size_t)ptr - sizeof(struct chunk));
	ch->flags = FREE;

    // Merge consecutive free chunks
	for (struct chunk *item = heap;
			(size_t)item < (size_t)heap + heap_size;
			item = (struct chunk *)((size_t)item + item->size + sizeof(struct chunk))
			)
	{
		printf("Chunk check %p size %lu - flag %u\n", item, item->size, item->flags);
		if (item->flags == FREE)
		{
            // Check for consecutive free blocks
			struct chunk *end = item;
			size_t new_size = item->size;
			while (end->flags == FREE && (size_t)end + sizeof(struct chunk) + end->size < (size_t) heap + heap_size)
			{
				end = (struct chunk*)((size_t)end + end->size + sizeof(struct chunk));
				if (end->flags == FREE)
				{
					new_size += end->size + sizeof(struct chunk);
				}
				printf("new size: %lu consecutive blocks %p size %lu - flag %u\n", new_size, end, end->size, end->flags);
			}
			item->size = new_size;
		}
	}
}
