#include "secmalloc.private.h"
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
size_t heap_size = 4096; // used as constant
size_t heapdata_size = 4096; // will increase
size_t heapmetadata_size = 4096; // will increase

void *init_heapdata()
{
	if (heapdata == NULL)
	{
		heapdata = mmap((void*)(4096*100000), heap_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	}
	return heapdata;
}

struct chunkmetadata *init_heapmetadata()
{
	if (heapmetadata == NULL)
	{
		heapmetadata = (struct chunkmetadata*) mmap((void*)(4096*1000), heap_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
		heapmetadata->size = heap_size - sizeof(struct chunkmetadata);
		heapmetadata->flags = FREE;
		heapmetadata->addr = heapdata;
		heapmetadata->canary = 0xdeadbeef; // will be replaced by a random value during first malloc
		heapmetadata->next = NULL;
		heapmetadata->prev = NULL;
	}
	return heapmetadata;
}


/* struct chunk *init_heap() */
/* { */
/* 	if (heap == NULL) */
/* 	{ */
/* 		heap = (struct chunk*) mmap((void*)(4096*100000), heap_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0); */
/* 		// heap = (struct chunk*) mmap(NULL, heap_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0); */
/* 		heap->size = heap_size - sizeof(struct chunk); */
/* 		heap->flags = FREE; */
/* 	} */
/* 	return heap; */
/* } */

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
	
/* struct chunk *get_last_chunk_raw() */
/* { */
/* 	for (struct chunk *item = heap; */
/* 			(size_t)item < (size_t) heap + heap_size; */
/* 			item = (struct chunk *)((size_t)item + sizeof(struct chunk) + item->size) */
/* 			) */
/* 	{ */
/* 		printf("last chunk check %p size %lu - flag %u\n", item, item->size, item->flags); */
/* 		if ((size_t)item + sizeof(struct chunk) + item->size >= (size_t) heap + heap_size) */
/* 		{	 */
/* 			printf("ret %p\n", item); */
/* 			return item; */
/* 		} */
/* 		printf("skip\n"); */
/* 	} */
/* 	return NULL; */
/* } */
/*  */
/* struct chunk *get_free_chunk_raw(size_t size) */
/* { */
/* 	if (heap == NULL) */
/* 		heap = init_heap(); */
/* 	for (struct chunk *item = heap; */
/* 			(size_t)item < (size_t)heap + heap_size; */
/* 			item = (struct chunk *)((size_t)item + item->size + sizeof(struct chunk)) */
/* 			) */
/* 	{ */
/* 		if (item->flags == FREE && item->size >= size) */
/* 			return item; */
/* 	} */
/* 	return NULL; */
/* } */
/*  */
/*  */
/* struct chunk *get_free_chunk(size_t size) */
/* { */
/* 	if (heap == NULL) */
/* 		heap = init_heap(); */
/* 	printf("heap %p\n", heap); */
/* 	struct chunk *item = get_free_chunk_raw(size); */
/* 	if (item == NULL) */
/* 	{ */
/* 		//manque d'espace memoire REMAP */
/* 		printf("HERE %p\n", item); */
/* 		size_t tot_size = size + sizeof(struct chunk); */
/* 		size_t old_size = heap_size; */
/* 		size_t delta_size = ((tot_size/4096) + ((tot_size % 4096 != 0) ? 1 : 0)) * 4096; */
/* 		struct chunk *last_item = get_last_chunk_raw(); */
/* 		heap_size += delta_size; */
/* 		printf("HEAP NEW SIZE %lu\n", heap_size); */
/* 		struct chunk *new_heap = mremap(heap, old_size, heap_size, MREMAP_MAYMOVE); */
/* 		printf("HEAP resized %p\n", new_heap); */
/* 		if (new_heap != heap) */
/* 			return NULL; // pour verifier qu'on s'est pas fait deporter */
/* 		printf("LAST SIZE %lu - %p\n", delta_size, last_item); */
/* 		last_item->size += delta_size;  */
/* 		printf("last chunk %p size %lu - flag %u\n", last_item, last_item->size, last_item->flags); */
/* 		item = get_free_chunk_raw(size); */
/* 		printf("item chunk %p\n", item); */
/* 	} */
/* 	return item; */
/* } */
/*  */
/* void *my_alloc(size_t size) { */
/* 	(void) size; */
/* 	void *ptr; */
/* 	// get free chunk */
/* 	struct chunk *ch = get_free_chunk(size); */
/* 	// split chunk */
/* 	ptr =(void*) ((size_t)ch + sizeof(struct chunk)); */
/* 	// get end of chunk */
/* 	struct chunk *end = (struct chunk*)((size_t)ptr + size); */
/* 	end->flags = FREE; */
/* 	end->size = ch->size - sizeof(struct chunk) - size; */
/* 	ch->flags = BUSY; */
/* 	ch->size = size; */
/* 	return ptr; */
/* } */
/*  */
/* void clean(void *ptr) */
/* { */
/* 	struct chunk *ch = (struct chunk*)((size_t)ptr - sizeof(struct chunk)); */
/* 	// ?? si ptr cest nimp */
/* 	// lookp ptr : idee de truc secu a faire */
/* 	ch->flags = FREE; */
/* 	// merge les chunks consecutifs */
/* 	for (struct chunk *item = heap; */
/* 			(size_t)item < (size_t)heap + heap_size; */
/* 			item = (struct chunk *)((size_t)item + item->size + sizeof(struct chunk)) */
/* 			) */
/* 	{ */
/* 		printf("Chunk check %p size %lu - flag %u\n", item, item->size, item->flags); */
/* 		if (item->flags == FREE) */
/* 		{ */
/* 			// voir les blocs consecutifs */
/* 			struct chunk *end = item; */
/* 			size_t new_size = item->size; */
/* 			while (end->flags == FREE && (size_t)end + sizeof(struct chunk) + end->size < (size_t) heap + heap_size) */
/* 			{ */
/* 				end = (struct chunk*)((size_t)end + end->size + sizeof(struct chunk)); */
/* 				if (end->flags == FREE) */
/* 				{ */
/* 					new_size += end->size + sizeof(struct chunk); */
/* 				} */
/* 				printf("new size: %lu consecutive blocks %p size %lu - flag %u\n", new_size, end, end->size, end->flags); */
/* 			} */
/* 			item->size = new_size; */
/* 		} */
/* 	} */
/* } */

// get the total size of the heapmetadata
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

// get the total size of the heapdata
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

// resize the heapmetadata
void resizeheapmetadata()
{
	heapmetadata = mremap(heapmetadata, heapmetadata_size, heap_size, MREMAP_MAYMOVE);
	heapmetadata_size += heap_size;
	return;
}

// resize the heapdata
void resizeheapdata()
{
	heapdata = mremap(heapdata, heapdata_size, heap_size, MREMAP_MAYMOVE);
	heapdata_size += heap_size;
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

// split a bloc in two, returns the new second bloc 
void split(struct chunkmetadata *bloc, size_t size)
{
	//create new metadata bloc for the second part
	struct chunkmetadata *newbloc = lastmetadata();
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
	long *canary_ptr = (long*)((size_t)bloc->addr + bloc->size);
	*canary_ptr = canary;
}

// malloc
void* my_malloc(size_t size)
{
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
	size_t heapmetadata_size = get_heapmetadata_size();
	if (4096 - heapmetadata_size % 4096 < sizeof(struct chunkmetadata)) 
	{
		resizeheapmetadata();
	}
	// get the total size of data heap and resize it if needed
	size_t heapdata_size = get_heapdata_size();
	if (4096 - heapdata_size % 4096 < size)
	{
		resizeheapdata();
	}
	// get metadata bloc of free data bloc with large enough size 
	struct chunkmetadata *bloc = lookup(size);
	// if no bloc found, return NULL
	if (bloc == NULL)
	{
		return NULL;
	}
	// generate a canary
	long canary = generate_canary();
	// split the bloc
	split(bloc, size);
	// fill the bloc with metadata 
	bloc->flags = BUSY;
	bloc->canary = canary;
	// place the canary at the end of the bloc data in heapdata
	place_canary(bloc, canary);
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
	log_message("Verifying canary");
	long expected_canary = item->canary;
	long *canary = (long*)((size_t)item->addr + item->size);
	if (*canary != expected_canary)
	{
		return -1;
	}
	log_message("Canary verified");
	return 1;
}

void clean_memory(struct chunkmetadata *item)
{
	log_message("Cleaning memory");
	memset(item->addr, 0, item->size);
	log_message("Memory cleaned");
}

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

void my_free(void *ptr)
{
	log_event(FREE_fn, START, ptr, 0);
	// we check if the heap is initialized
	if (heapdata == NULL || heapmetadata == NULL)
	{
		log_message("Heap not initialized");
		return;
	}
	// if ptr is NULL we log an error
	if (ptr == NULL)
	{
		log_message("Invalid pointer to free : NULL");
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
				log_message("Double free");
				return;
			}
			// if the canary is not the one we expect we log an error
			if (verify_canary(item) == -1)
			{
				log_message("Canary verification failed : Buffer overflow detected");
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
	log_message("Invalid pointer to free : not in the heap");
	return;
}

// Vérification
