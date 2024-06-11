#ifndef __MY_ALLOC_H__
#define __MY_ALLOC_H__
#include <stddef.h>

void *my_malloc(size_t size); //size_t for unsigned int
void clean(void *ptr);

#endif