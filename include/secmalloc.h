#ifndef __MY_ALLOC_H__
#define __MY_ALLOC_H__
#include <stddef.h>

void *my_alloc(size_t size); //size_t is unsigned int 
void clean(void *ptr);


#endif // __MY_ALLOC_H__
