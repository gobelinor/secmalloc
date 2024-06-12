#ifndef __MY_ALLOC_H__
#define __MY_ALLOC_H__
#include <stddef.h>
#include "secmalloc.private.h"

void* my_malloc(size_t size);
void my_free(void* ptr);

#endif // __MY_ALLOC_H__
