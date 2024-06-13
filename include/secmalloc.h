#ifndef __MY_ALLOC_H__
#define __MY_ALLOC_H__
#include <stddef.h>
#include "secmalloc.private.h"

void* my_malloc(size_t size);
void my_free(void* ptr);
void* my_calloc(size_t nmemb, size_t size);
void* my_realloc(void* ptr, size_t size);

#endif // __MY_ALLOC_H__
