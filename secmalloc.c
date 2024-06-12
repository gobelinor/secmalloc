#include "secmalloc.h"
#define _GNU_SOURCE
#include <fcntl.h>
#include <stdio.h>
#include <sys/mman.h>
#include <linux/mman.h>
#include <unistd.h>
#include <string.h>
#include "log.h"

void *heapdata = NULL;
struct chunkmetadata *heapmetadata = NULL;
size_t pageheap_size = 4096; // used as constant
size_t heapdata_size = 4096; // will increase
size_t heapmetadata_size = 4096; // will increase

void *init_heapdata()
{
	if (heapdata == NULL)
	{
		heapdata = mmap((void*)(4096*100000), pageheap_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	}
	return heapdata;
}

struct chunkmetadata *init_heapmetadata()
{
	if (heapmetadata == NULL)
	{
		heapmetadata = (struct chunkmetadata*) mmap((void*)(4096*1000), pageheap_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
		heapmetadata->size = pageheap_size;
		heapmetadata->flags = FREE;
		heapmetadata->addr = heapdata;
		heapmetadata->canary = 0xdeadbeef; // will be replaced by a random value during first malloc
		heapmetadata->next = NULL;
		heapmetadata->prev = NULL;
	}
	return heapmetadata;
}


long generate_canary()
{
	long canary = 0;
	int fd = open("/dev/urandom", O_RDONLY);
	if (fd == -1)
	{
		return -1;
	}
	if (read(fd, &canary, sizeof(long)) == -1)
	{
		return -1;
	}
	close(fd);
	return canary;
}

// get the total size of the heapmetadata
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

// get the total size of the heapdata
size_t get_allocated_heapdata_size()
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

// get the last metadata bloc
struct chunkmetadata *lastmetadata()
{
	struct chunkmetadata *item = heapmetadata;
	while (item->next != NULL)
	{
		item = item->next;
	}
	return item;
}

// resize the heapmetadata
void resizeheapmetadata()
{
	heapmetadata = mremap(heapmetadata, heapmetadata_size, heapmetadata_size + pageheap_size, MREMAP_MAYMOVE);
	heapmetadata_size += pageheap_size;
	return;
}

// resize the heapdata
void resizeheapdata()
{
	heapdata = mremap(heapdata, heapdata_size, heapdata_size+pageheap_size, MREMAP_MAYMOVE);
	heapdata_size += pageheap_size;
	struct chunkmetadata *last = lastmetadata();
	last->size += pageheap_size;
	/* printf("last->size : %ld\n", last->size); */
	return;
}

// lookup for a free bloc with a size large enough
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


// split a bloc in two, returns the new second bloc 
void split(struct chunkmetadata *bloc, size_t size)
{
	//create new metadata bloc for the second part
	struct chunkmetadata *newbloc = (struct chunkmetadata*) ((size_t)lastmetadata()+sizeof(struct chunkmetadata));
	//set metadata for newbloc
	newbloc->size = bloc->size - size;
	newbloc->flags = FREE;
	newbloc->addr = (void*)((size_t)bloc->addr + size);
	newbloc->canary = 0xdeadbeef;
	newbloc->next = bloc->next;
	newbloc->prev = bloc;
	//update first bloc metadata
	bloc->size = size;
	bloc->next = newbloc;
	return;
}

void place_canary(struct chunkmetadata *bloc, long canary)
{
	/* printf("bloc->addr : %p\n", bloc->addr); */
	long *canary_ptr = (long*)((size_t)bloc->addr + bloc->size);
	/* printf("canary_ptr : %p\n", canary_ptr); */
	*canary_ptr = canary;
	/* printf("canary : %ld\n", *canary_ptr); */
}

// malloc
void* my_malloc(size_t size)
{
	// if requested size is 0, return NULL
	if (size == 0)
	{
		return NULL;
	}
	// check if the heap is initialized
	if (heapdata == NULL)
	{
		init_heapdata();
	}
	// check if the heapmetadata is initialized
	if (heapmetadata == NULL)
	{
		init_heapmetadata();
	}
	// get the total size of metadata heap and resize it if needed
	size_t allocated_heapmetadata_size = get_allocated_heapmetadata_size();
	if (4096 - allocated_heapmetadata_size % 4096 < sizeof(struct chunkmetadata)) 
	{
		/* printf("resizeheapmetadata\n"); */
		resizeheapmetadata();
	}
	// get the total size of data heap and resize it if needed
	size_t allocated_heapdata_size = get_allocated_heapdata_size();
	if (4096 - allocated_heapdata_size % 4096 < size)
	{
		/* printf("resizeheapdata\n"); */
		resizeheapdata();
	}
	// get metadata bloc of free data bloc with large enough size 
	struct chunkmetadata *bloc = lookup(size);
	/* printf("post lookup\n"); */
	/* printf("bloc->addr : %p\n", bloc->addr); */
	/* printf("bloc->size : %ld\n", bloc->size); */
	/* printf("bloc->flags : %d\n", bloc->flags); */
	/* printf("bloc->canary : %ld\n", bloc->canary); */
	/* printf("bloc->next : %p\n", bloc->next); */
	/* printf("bloc->prev : %p\n", bloc->prev); */
	// if no bloc found, return NULL
	if (bloc == NULL)
	{
		return NULL;
	}
	// generate a canary
	long canary = generate_canary();
	// split the bloc
	split(bloc, size);
	/* printf("post split\n"); */
	/* printf("bloc->addr : %p\n", bloc->addr); */
	/* printf("bloc->size : %ld\n", bloc->size); */
	/* printf("bloc->flags : %d\n", bloc->flags); */
	/* printf("bloc->canary : %ld\n", bloc->canary); */
	/* printf("bloc->next : %p\n", bloc->next); */
	/* printf("bloc->prev : %p\n", bloc->prev); */
	/* printf("newbloc->addr : %p\n", bloc->next->addr); */
	/* printf("newbloc->size : %ld\n", bloc->next->size); */
	/* printf("newbloc->flags : %d\n", bloc->next->flags); */
	/* printf("newbloc->canary : %ld\n", bloc->next->canary); */
	/* printf("newbloc->next : %p\n", bloc->next->next); */
	/* printf("newbloc->prev : %p\n", bloc->next->prev); */
	// fill the bloc with metadata 
	bloc->flags = BUSY;
	bloc->canary = canary;
	// place the canary at the end of the bloc data in heapdata
	/* printf("pre place canary\n"); */
	place_canary(bloc, canary);
	/* printf("post place canary\n"); */
	// return the address of the data bloc in heapdata
	return bloc->addr;
}
	
	



// not used because included in free
int is_valid(void *ptr)
{
	// check if ptr is in heapdata
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

int verify_canary(struct chunkmetadata *item)
{
	log_message("Verifying canary\n");
	long expected_canary = item->canary;
	long *canary = (long*)((size_t)item->addr + item->size);
	if (*canary != expected_canary)
	{
		return -1;
	}
	log_message("Canary verified\n");
	return 1;
}

void clean_memory(struct chunkmetadata *item)
{
	log_message("Cleaning memory\n");
	memset(item->addr, 0, item->size);
	log_message("Memory cleaned\n");
}

void merge_chunks()
{
	log_message("Merging chunks\n");
	// we iterate over the heapmetadata to merge free chunks
	printf("Merge chunks\n");
	for (struct chunkmetadata *item = heapmetadata;
			item != NULL;
			item = item->next)
	{
		printf("item->addr : %p\n", item->addr);
		// if the chunk is free we merge it with the next one if it is free
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
					item->size = new_size;
					item->next = end->next;
				}	
			}
			if (end->next != NULL)
			{
				end->next->prev = item;
			}
			if (count > 0)
			{
				log_message("%d chunks merged\n", count);
			}
		}
	}
}

void my_free(void *ptr)
{
	log_event(FREE_fn, START, ptr, 0);
	// we check if the heap is initialized
	if (heapdata == NULL || heapmetadata == NULL)
	{
		log_message("Heap not initialized\n");
		return;
	}
	// if ptr is NULL we log an error
	if (ptr == NULL)
	{
		log_message("Invalid pointer to free : NULL\n");
		return;
	}
	// first we verify if ptr is one of the addresses where we allocated memory
	for (struct chunkmetadata *item = heapmetadata;
			item != NULL;
			item = item->next)
	{
		// if ptr is the one of the known addr in the heapmetadata we free it
		if (item->addr == ptr)
		{
			// if the chunk is already free we log an error
			if (item->flags == FREE)
			{
				log_message("Double free\n");
				return;
			}
			// if the canary is not the one we expect we log an error
			if (verify_canary(item) == -1)
			{
				log_message("Canary verification failed : Buffer overflow detected\n");
				return;
			}
			// we clean the memory before freeing it
			clean_memory(item);
			// we free the chunk
			item->flags = FREE;
			// we log the event
			log_event(FREE_fn, END, ptr, item->size);
			// we merge free chunks
			merge_chunks();
			return;
		}
	}
	// if ptr is not in the heap, we log an error
	log_message("Invalid pointer to free : not in the heap\n");
	return;
}

// Vérification
