#ifndef __SECMALLOC_H__
#define __SECMALLOC_H__
#include <stddef.h>

void *my_malloc(size_t size);
void clean(void *ptr);

#endif
