#include <stdio.h>
#include "my_alloc.h"

int main()
{
    // Allocate 12 bytes of memory
    char *ptr1 = my_malloc(12);
    if (ptr1 == NULL) {
        fprintf(stderr, "Memory allocation failed for ptr1\n");
        return 1;
    }
    printf("ptr1 = %p\n", ptr1);

    // Allocate 25 bytes of memory
    char *ptr2 = my_malloc(25);
    if (ptr2 == NULL) {
        fprintf(stderr, "Memory allocation failed for ptr2\n");
        clean(ptr1); // Clean up previously allocated memory before exiting
        return 1;
    }
    printf("ptr2 = %p\n", ptr2);

    // Print the difference between the two pointers
    printf("ptr2 - ptr1 = %ld\n", (size_t)ptr2 - (size_t)ptr1);

    // Clean up allocated memory
    clean(ptr1);
    clean(ptr2);

    // Calculate the chunk structure from ptr2
    struct chunk *t = (struct chunk*)((size_t)ptr2 - sizeof(struct chunk));
    printf("t->size = %ld\n", t->size);

    return 0;
}
