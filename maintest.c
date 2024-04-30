#include <stdio.h>
#include "my_alloc.h"

int main()
{
	char *ptr1 = my_alloc(12);
	printf("ptr1 = %p\n", ptr1);
	char *ptr2 = my_alloc(25);
	printf("ptr2 = %p\n", ptr2);
	clean(ptr1);
	clean(ptr2);
	struct chunk *t = (struct chunk*)((size_t)ptr2 - sizeof(struct chunk));
	printf("t->size = %ld\n", t->size);
	return 0;
}
