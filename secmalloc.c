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
void resizeheapdata(size_t new_size)
{
	heapdata = mremap(heapdata, heapdata_size, new_size, MREMAP_MAYMOVE);
	heapdata_size = new_size;
	struct chunkmetadata *last = lastmetadata();
	last->size = new_size;
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
		if (item->flags == FREE && item->size >= size + sizeof(long))
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
	newbloc->size = bloc->size - size - sizeof(long);
	newbloc->flags = FREE;
	newbloc->addr = (void*)((size_t)bloc->addr + size + sizeof(long));
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
	/* printf("allocated_heapmetadata_size : %ld\n", allocated_heapmetadata_size); */
	if (4096 - allocated_heapmetadata_size % 4096 < sizeof(struct chunkmetadata)) 
	{
		/* printf("resizeheapmetadata\n"); */
		resizeheapmetadata();
	}
	// get the total size of data heap and resize it if needed
	size_t allocated_heapdata_size = get_allocated_heapdata_size();
	/* printf("allocated_heapdata_size : %ld\n", allocated_heapdata_size); */
	size_t needed_size = size + sizeof(long);
	size_t available_size = heapdata_size - allocated_heapdata_size;
	if (available_size < needed_size)
	{
		/* printf("resizeheapdata\n"); */
		size_t new_size = allocated_heapdata_size + needed_size;
		new_size = ((new_size/4096) + ((new_size % 4096 != 0) ? 1 : 0))*4096;
		/* printf("new_size : %ld\n", new_size); */
		resizeheapdata(new_size);
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
	/* printf("Merge chunks\n"); */
	for (struct chunkmetadata *item = heapmetadata;
			item != NULL;
			item = item->next)
	{
		/* printf("item->addr : %p\n", item->addr); */
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

void* my_calloc(size_t nmemb, size_t size)
{
	// we check if the heap is initialized
	if (heapdata == NULL)
	{
		init_heapdata();
	}
	// we check if the heapmetadata is initialized
	if (heapmetadata == NULL)
	{
		init_heapmetadata();
	}
	if (nmemb == 0 || size == 0)
	{
		return NULL;
	}
	// we allocate memory
	void *ptr = my_malloc(nmemb*size);
	// we set the memory to 0
	memset(ptr, 0, nmemb*size);
	// we return the pointer
	return ptr;
}

void *my_realloc(void *ptr, size_t size)
{
	// we check if the heap is initialized
	if (heapdata == NULL)
	{
		init_heapdata();
	}
	// we check if the heapmetadata is initialized
	if (heapmetadata == NULL)
	{
		init_heapmetadata();
	}
	// if ptr is NULL we call malloc
	if (ptr == NULL)
	{
		return my_malloc(size);
	}
	// if size is 0 we call free
	if (size == 0)
	{
		my_free(ptr);
		return NULL;
	}
	// we look for the metadata bloc corresponding to ptr
	for (struct chunkmetadata *item = heapmetadata;
			item != NULL;
			item = item->next)
	{
		if (item->addr == ptr)
		{
			// if the canary is not the one we expect we log an error
			if (verify_canary(item) == -1)
			{
				log_message("Realloc : Canary verification failed : Buffer overflow detected\n");
			}
			// we allocate memory
			void *new_ptr = my_malloc(size);
			// we copy the data from the old memory to the new one
			memcpy(new_ptr, ptr, item->size);
			// we free the old memory
			my_free(ptr);
			// we return the new pointer
			return new_ptr;
		}
	}
	// if ptr is not in the heap, we log an error
	log_message("Invalid pointer to realloc : not in the heap\n");
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
