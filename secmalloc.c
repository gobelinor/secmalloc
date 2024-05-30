#include "secmalloc.private.h"
#define _GNU_SOURCE
#include <fcntl.h>
#include <stdio.h>
#include <sys/mman.h>
#include <linux/mman.h>
#include <unistd.h>


void *heapdata = NULL;
struct chunkmetadata *heapmetadata = NULL;
size_t heap_size = 4096;

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
