#ifndef __MY_ALLOC_H__
#define __MY_ALLOC_H__
#include <stddef.h>

// enum chunk_type 
// {
// 	FREE = 0,
// 	BUSY = 1
// };
// struct chunk 
// {
// 	size_t size;
// 	enum chunk_type flags;
// };

void *my_alloc(size_t size); //size_t is unsigned int 
void clean(void *ptr);

// struct chunk *init_heap();
// struct chunk *get_free_chunk(size_t size);

#endif // __MY_ALLOC_H__
