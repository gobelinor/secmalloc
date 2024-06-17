/**
 * @file test.c
 * @brief Unit tests for secure memory allocation functions.
 *
 * This file contains unit tests for the secure memory allocation functions using Criterion framework.
 */


#include <criterion/criterion.h>
#include <sys/mman.h>
#include "secmalloc.h"
#include "log.h"
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>

/***** Begin of simples tests mmap *****/

/**
 * @brief Test simple mmap allocation.
 */
Test(simple, simple_map_01)
{
    // Simple use of mmap
    char    *ptr = mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    cr_assert(ptr != NULL, "Failed to mmap");
}

/**
 * @brief Test mmap with a larger size.
 */
Test(simple, simple_map_02)
{
    // mmap with a larger size
    // printf("Testing mmap with larger size 8192 bytes\n");
    char    *ptr = mmap(NULL, 8192, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    cr_assert(ptr != MAP_FAILED, "Failed to mmap 8192 bytes");
    // printf("Successfully mapped 8192 bytes, now unmapping...\n");
    munmap(ptr, 8192);
}

/**
 * @brief Test mmap to request a minimal memory page.
 */
Test(simple, simple_map_03)
{
    // mmap to request a minimal memory page
    // printf("Testing mmap with system page size\n");
    char    *ptr = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    cr_assert(ptr != MAP_FAILED, "Failed to mmap one system page size");
    // printf("Successfully mapped one system page size, now unmapping...\n");
    munmap(ptr, sysconf(_SC_PAGESIZE));
}

/**
 * @brief Test mmap with specific memory alignment.
 */
Test(simple, simple_map_04)
{
    // mmap with specific memory alignment
    size_t    alignment = 16384; // 16KB alignment
    size_t    size = 4096;
    // printf("Testing mmap with specific alignment: 16KB over 4KB size\n");
    char    *ptr = mmap(NULL, size + alignment, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    char    *aligned_ptr = (char *)(((uintptr_t)ptr + alignment - 1) & ~(alignment - 1));
    cr_assert(aligned_ptr >= ptr && aligned_ptr < ptr + alignment, "Failed to get aligned memory");
    // printf("Alignment successful, ptr: %p, aligned_ptr: %p\n", ptr, aligned_ptr);
    munmap(ptr, size + alignment);
}

/**
 * @brief Test mmap with write-only protection.
 */
Test(simple, simple_map_5)
{
    // mmap with write-only protection
    // printf("Testing mmap with write-only protection\n");
    char    *ptr = mmap(NULL, 4096, PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    cr_assert(ptr != MAP_FAILED, "Failed to mmap with write-only protection");
    // printf("Write-only mmap successful, now unmapping...\n");
    munmap(ptr, 4096);
}

/**
 * @brief Test mmap with an invalid file descriptor.
 */
Test(simple, simple_map_6)
{
    // mmap with an invalid file descriptor
    // printf("Testing mmap with an invalid file descriptor\n");
    char    *ptr = mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_PRIVATE, -1, 0);
    cr_assert(ptr == MAP_FAILED, "mmap should fail with an invalid file descriptor");
    if (ptr != MAP_FAILED)
    {
        // printf("Unexpected success, now unmapping...\n");
        munmap(ptr, 4096);
    }
    else
    {
        // printf("Failed as expected with invalid file descriptor\n");
    }
}

/* ***** End of simples tests mmap ***** */


/* ***** Begin of simples tests log ***** */

/**
 * @brief Test logging a message.
 */
Test(simple, log_02)
{
    // printf("log_02\n");
    int    ret = my_log_message("ecrit ce que tu %s : %d\n", "veux", 12);
    // printf("ret = %d\n", ret);
    cr_assert(ret == 0);
}

/* ***** End of simples tests log ***** */


/* ***** Begin of simples tests canary ***** */

/**
 * @brief Test canary generation.
 */
Test(simple, canary_01)
{
    // printf("canary_01\n");
    long    ret = my_generate_canary();
    // printf("canary = %ld\n", ret);
    // printf("sizeofcanary = %ld\n", sizeof(long));
    // printf("sizeofaddr = %ld\n", sizeof(void *));
    cr_assert(ret != -1);
    cr_assert(sizeof(long) == sizeof(void *));
}

/**
 * @brief Test canary uniqueness.
 */
Test(simple, canary_02) // not perfect but that should be enough for our purpuses
{
    // printf("canary_02\n");
    for (int i = 0; i < 100; i++)
    {
	    long ret1 = my_generate_canary();
	    cr_assert(ret1 != -1);
	    long ret2 = my_generate_canary();
        cr_assert(ret2 != -1);
	    cr_assert(ret1 != ret2);
    }
}

/**
 * @brief Test canary placement and verification.
 */
Test(simple, canary_03)
{
    /* printf("canary_03\n"); */
    void    *ptr1 = my_malloc(100);
    cr_assert(ptr1 != NULL);
    cr_assert(heapmetadata->canary == *((long *)((size_t)heapdata + 100)));

    void    *ptr2 = my_malloc(200);
    cr_assert(ptr2 != NULL);
    cr_assert(heapmetadata->next->canary == *((long *)((size_t)heapdata + 300 + sizeof(long))));
    cr_assert(heapmetadata->canary == *((long *)((size_t)heapdata + 100)));
}

/**
 * @brief Test canary verification failure.
 */
Test(simple, canary_04)
{
    /* printf("canary_04\n"); */
    void    *ptr1 = my_malloc(100);
    cr_assert(ptr1 != NULL);
    *((long *)((size_t)ptr1 + 100)) = 0xdeadbeef;
    my_free(ptr1); // will not free because wrong canary
    cr_assert(heapmetadata->flags == FREE);
}

/* ***** End of simples tests canary ***** */


/* ***** Begin of simples tests heap ***** */

/**
 * @brief Test heap initialization.
 */
Test(simple, init_heaps_01)
{
    // printf("init_heaps_01\n");
    heapdata = NULL;
    heapmetadata = NULL;
    heapdata = my_init_heapdata();
    heapmetadata = my_init_heapmetadata();
    cr_assert(heapmetadata != NULL);
    cr_assert(heapdata != NULL);
    cr_assert(heapmetadata->size == PAGE_HEAP_SIZE);
    cr_assert(heapmetadata->flags == FREE);
    cr_assert(heapmetadata->addr == heapdata);
    cr_assert(heapmetadata->canary == 0xdeadbeef);
    cr_assert(heapmetadata->next == NULL);
}

/* ***** End of simples tests heap ***** */


/* ***** Begin of simples tests malloc ***** */

/**
 * @brief Test malloc allocation.
 */
Test(simple, my_malloc_01)
{
    /* printf("my_malloc_01\n"); */
    void    *ptr = my_malloc(100);
    cr_assert(ptr != NULL);
    cr_assert(ptr == heapdata);
    // verify the first metadata bloc (heapmetadata)
    cr_assert(heapmetadata->addr == ptr);
    cr_assert(heapmetadata->size == 100);
    cr_assert(heapmetadata->flags == BUSY);
    long    canary = *((long *)((size_t)ptr + 100));
    cr_assert(heapmetadata->canary == canary);
    /* printf("heapmetadata->next = %p\n", heapmetadata->next); */
    /* printf("heapmetadata = %p\n", heapmetadata); */
    /* printf("sizeof(struct chunkmetadata) = %ld\n", sizeof(struct chunkmetadata)); */
    /* printf("heapmetadata + sizeof(struct chunkmetadata) = %p\n", */
    /* (void*)((size_t)heapmetadata + sizeof(struct chunkmetadata))); */
    /* printf("heapmetadata->next = %p\n", heapmetadata->next); */
    /* printf("(void *) ((size_t)heapmetadata + sizeof(struct chunkmetadata))) = %p\n", (void *) */
    /*((size_t)heapmetadata + sizeof(struct chunkmetadata))); */
    cr_assert(heapmetadata->next == (void *) ((size_t)heapmetadata + sizeof(struct chunkmetadata)));
    // verify the next metadata bloc
    cr_assert(heapmetadata->next->size == PAGE_HEAP_SIZE - 100 - sizeof(long));
    cr_assert(heapmetadata->next->flags == FREE);
    cr_assert(heapmetadata->next->addr == (void *) ((size_t)heapdata + 100 + sizeof(long)));
    cr_assert(heapmetadata->next->canary == 0xdeadbeef);
    cr_assert(heapmetadata->next->next == NULL);
}

/**
 * @brief Test malloc with size 0.
 */
Test(simple, my_malloc_02)
{
    // printf("my_malloc_02\n");
    void    *ptr = my_malloc(0);
    cr_assert(ptr == NULL);
    // printf("ptr = %p\n", ptr);
}

/**
 * @brief Test consecutive malloc allocations.
 */
Test(simple, my_malloc_03)
{
    heapdata = NULL;
    heapmetadata = NULL;
	/* printf("my_malloc_03\n"); */
	/* printf("heapdata = %p\n", heapdata); */
	/* printf("metadataheap = %p\n", heapmetadata); */
    void    *ptr1 = my_malloc(100);
	/* printf("metadataheap = %p\n", heapmetadata); */
	/* printf("heapdata = %p\n", heapdata); */
	/* printf("ptr1 = %p\n", ptr1); */
    cr_assert(ptr1 != NULL);
    void    *ptr2 = my_malloc(100);
	/* printf("ptr2 = %p\n", ptr2); */
    cr_assert(ptr2 != NULL);
    cr_assert(ptr1 != ptr2);
}

/**
 * @brief Test malloc with larger size.
 */
Test(simple, my_malloc_04)
{
	void    *ptr = my_malloc(1000);
	cr_assert(ptr != NULL);
	long    canary = *((long *)((size_t)ptr + 1000));
	cr_assert(heapmetadata->canary == canary);
}

/**
 * @brief Test multiple malloc allocations.
 */
Test(simple, my_malloc_05)
{
	/* printf("my_malloc_05\n"); */
	void    *ptr1 = my_malloc(4096);
	/* printf("ptr1 = %p\n", ptr1); */
	void    *ptr2 = my_malloc(4096);
	/* printf("ptr2 = %p\n", ptr2); */
	void    *ptr3 = my_malloc(4096);
	/* printf("ptr3 = %p\n", ptr3); */
	cr_assert(ptr1 != NULL);
	cr_assert(heapmetadata->size == 4096);
	cr_assert(heapmetadata->next->addr == ptr2);
	cr_assert(ptr2 != NULL);
	cr_assert(heapmetadata->next->size == 4096);
	cr_assert(heapmetadata->next->next->addr == ptr3);
	cr_assert(ptr3 != NULL);
}

/**
 * @brief Test malloc with size close to page size.
 */
Test(simple, my_malloc_06)
{
	/* printf("my_malloc_06\n"); */
	void    *ptr1 = my_malloc(4090);
	/* printf("ptr1 = %p\n", ptr1); */
	cr_assert(ptr1 != NULL);
}

/**
 * @brief Test malloc with size just below page size.
 */
Test(simple, my_malloc_07)
{
	/* printf("my_malloc_06\n"); */
	void    *ptr = my_malloc(4096-sizeof(long));
	cr_assert (ptr != NULL);
	void    *ptr2 = my_malloc(666);
	cr_assert(heapmetadata->next->addr == ptr2);
}

/* ***** End of simples tests malloc ***** */


/* ***** Begin of simples tests free ***** */

/**
 * @brief Test freeing a malloc-ed block.
 */
Test(simple, free_01)
{
	void    *ptr = my_malloc(100);
	cr_assert(ptr != NULL);
	my_free(ptr);
	cr_assert(heapmetadata->flags == FREE);
}

/**
 * @brief Test freeing a malloc-ed block and checking metadata.
 */
Test(simple, free_02)
{
	void    *ptr = my_malloc(100);
	cr_assert(ptr != NULL);
	my_free(ptr);
	cr_assert(heapmetadata->next == NULL);
}

/**
 * @brief Test freeing an invalid pointer.
 */
Test(simple, free_03)
{
	void    *ptr = (void*)0xdeadbeef;
	my_free(ptr);
	cr_assert(1==1);
}

/**
 * @brief Test freeing a valid and an invalid pointer.
 */
Test(simple, free_04)
{
	void    *ptr666 = my_malloc(100); //initialise heaps
	void    *ptr = (void*)0xdeadbeef;
	my_free(ptr);
	my_free(ptr666);
	cr_assert(1==1);
}

/**
 * @brief Test double free.
 */
Test(simple, free_05)
{
	void    *ptr = my_malloc(100);
    cr_assert(ptr != NULL);
	my_free(ptr);
	cr_assert(heapmetadata->flags == FREE);
	my_free(ptr);
	cr_assert(heapmetadata->flags == FREE);
}

/**
 * @brief Test freeing NULL pointer.
 */
Test(simple, free_06)
{
	void    *ptr = my_malloc(100);
	cr_assert(ptr != NULL);
	my_free(NULL);
	cr_assert(heapmetadata->flags == BUSY);
	my_free(ptr);
	cr_assert(heapmetadata->flags == FREE);
}

/* ***** End of simples tests free ***** */


/* ***** Begin of simples tests free / malloc ***** */

/**
 * @brief Test malloc and free cycle.
 */
Test(simple, malloc_free_01)
{
	void    *ptr = my_malloc(100);
	cr_assert(ptr != NULL);
	my_free(ptr);
	void    *ptr2 = my_malloc(100);
	cr_assert(ptr2 != NULL);
	cr_assert(ptr == ptr2);
}

/**
 * @brief Test malloc and free cycle with different pointers.
 */
Test(simple, malloc_free_02)
{
	void    *ptr = my_malloc(100);
	cr_assert(ptr != NULL);
	my_free(ptr);
	void    *ptr2 = my_malloc(100);
	cr_assert(ptr2 != NULL);
	my_free(ptr2);
}

/**
 * @brief Test malloc and free with multiple blocks.
 */
Test(simple, malloc_free_03)
{
	void    *ptr = my_malloc(100);
	cr_assert(ptr != NULL);
	void    *ptr2 = my_malloc(100);
	cr_assert(ptr2 != NULL);
	void    *ptr3 = my_malloc(100);
	my_free(ptr);
	my_free(ptr2);
	cr_assert(heapmetadata->next->addr == ptr3);
	my_free(ptr3);
}

/**
 * @brief Test malloc and free with large block sizes.
 */
Test(simple, malloc_free_04)
{
	/* printf("resize_03\n"); */
	/* log_message("\nresize_03\n"); */
	void    *ptr1 = my_malloc(3000);
	/* void *ptr2 = my_malloc(2000); */
	/* printf("ptr1 = %p\n", ptr1); */
	/* printf("heapmetadata->addr = %p\n", heapmetadata->addr); */
	/* printf("heapmetadata->size = %ld\n", heapmetadata->size); */
	/* printf("heapmetadata->flags = %d\n", heapmetadata->flags); */
	/* printf("heapmetadata->next->addr = %p\n", heapmetadata->next->addr); */
	/* printf("heapmetadata->next->size = %ld\n", heapmetadata->next->size); */
	/* printf("heapmetadata->next->flags = %d\n", heapmetadata->next->flags); */
	void    *ptr3 = my_malloc(1000);
	/* printf("ptr3 = %p\n", ptr3); */
	/* printf("heapmetadata->next->addr = %p\n", heapmetadata->next->addr); */
	/* printf("heapmetadata->next->size = %ld\n", heapmetadata->next->size); */
	/* printf("heapmetadata->next->flags = %d\n", heapmetadata->next->flags); */
	cr_assert(ptr1 != NULL);
	/* cr_assert(ptr2 != NULL); */
	cr_assert(ptr3 != NULL);
	my_free(ptr1);

	/* printf("post free\n"); */
	/* printf("ptr1 = %p\n", ptr1); */
	/* printf("heapmetadata->addr = %p\n", heapmetadata->addr); */
	/* printf("heapmetadata->size = %ld\n", heapmetadata->size); */
	/* printf("heapmetadata->flags = %d\n", heapmetadata->flags); */

	/* my_free(ptr2); */
	/* printf("ptr3 = %p\n", ptr3); */
	/* printf("heapmetadata->next->addr = %p\n", heapmetadata->next->addr); */
	/* printf("heapmetadata->next->size = %ld\n", heapmetadata->next->size); */
	/* printf("heapmetadata->next->flags = %d\n", heapmetadata->next->flags); */
	cr_assert(heapmetadata->addr == ptr1);
	cr_assert(heapmetadata->next->addr == ptr3);
	my_free(ptr3);
}

/**
 * @brief Test malloc and free with various sizes.
 */
Test(simple, malloc_free_05)
{
	void    *ptr1 = my_malloc(3000);
	void    *ptr2 = my_malloc(2000);
	void    *ptr3 = my_malloc(1000);
	cr_assert(ptr1 != NULL);
	cr_assert(ptr2 != NULL);
	cr_assert(ptr3 != NULL);
	my_free(ptr1);
	my_free(ptr2);
	void    *ptr4 = my_malloc(1000);
	cr_assert(ptr4 != NULL);
	cr_assert(heapmetadata->addr == ptr4);
	cr_assert(ptr4 == ptr1);
}

/* ***** End of simples tests free / malloc ***** */


/* ***** Begin of simples tests resize ***** */

/**
 * @brief Test malloc with size requiring heap resize.
 */
Test(simple, resize_01)
{
	/* printf("resize_01\n"); */
	void    *ptr = my_malloc(5000);
	/* printf("ptr = %p\n", ptr); */
	cr_assert(ptr != NULL);
}

/**
 * @brief Test malloc and free with size requiring heap resize.
 */
Test(simple, resize_02)
{
	/* printf("resize_02\n"); */
	void    *ptr = my_malloc(5000);
	/* printf("ptr = %p\n", ptr); */
	cr_assert(ptr != NULL);
	my_free(ptr);
}

/**
 * @brief Test multiple small mallocs to trigger heap resize.
 */
Test(simple, resize_03)
{
    void    *ptr = my_malloc(100);
	for (int i=0; i<100; i++)
	{
		ptr = my_malloc(3);
	}
	cr_assert(ptr != NULL);
}

/**
 * @brief Test multiple large mallocs to trigger heap resize.
 */
Test(simple, resize_06)
{
	extern struct    chunkmetadata *my_lastmetadata();
	void    *ptr = NULL;
	for (int i=0; i<100; i++) // 100 is ok but 1000 is not and i dont know why
	{
		ptr = my_malloc(300);
	}
	cr_assert(ptr != NULL);
	struct chunkmetadata *tmp = my_lastmetadata();
	cr_assert(tmp->flags == FREE);
}


/**
 * @brief Test mallocs with increasing size to trigger heap resize.
 */
Test(simple, resize_07)
{
	extern struct    chunkmetadata *my_lastmetadata();
	void    *ptr = NULL;
	ptr = malloc(10);
	/* printf("HELLO (size_t*)(heapdata - heapmetadata) = %ld\n"), ((size_t)(heapdata)
	- (size_t)heapmetadata); */
	// 200 is ok but 200 is not and i dont know why it is probably becaue there is not enough space
	// beatween heapdata and heapmetadata
	for (int i=0; i<200; i++)
	{
		ptr = my_malloc(300);
	}
	cr_assert(ptr != NULL);
	struct    chunkmetadata *tmp = my_lastmetadata();
	cr_assert(tmp->flags == FREE);
}

/**
 * @brief Test malloc with very large size to trigger heap resize.
 */
Test(simple, resize_05)
{
	/* printf("resize_05\n"); */
	void    *ptr = my_malloc(10000);
	cr_assert(ptr != NULL);
	cr_assert(heapmetadata->size == 10000);
	cr_assert(heapmetadata->flags == BUSY);
	/* printf("heapmetadata->next->size = %ld\n", heapmetadata->next->size); */
	cr_assert(heapmetadata->next->size == 4096*3-10000-sizeof(long));
	cr_assert(heapmetadata->next->flags == FREE);
	my_free(ptr);
	cr_assert(heapmetadata->flags == FREE);
}

/* ***** End of simples tests resize ***** */


/* ***** Begin of simples tests calloc ***** */

/**
 * @brief Test calloc with non-zero size.
 */
Test(simple, my_calloc_01)
{
	/* printf("my_calloc_01\n"); */
	void    *ptr = my_calloc(100, 1);
	cr_assert(ptr != NULL);
	cr_assert(heapmetadata->size == 100);
	cr_assert(heapmetadata->flags == BUSY);
	cr_assert(heapmetadata->next->size == 4096-100-sizeof(long));
	cr_assert(heapmetadata->next->flags == FREE);
}

/**
 * @brief Test calloc with zero elements.
 */
Test(simple, my_calloc_02)
{
	/* printf("my_calloc_02\n"); */
	void    *ptr = my_calloc(0, 1);
	cr_assert(ptr == NULL);
}

/**
 * @brief Test calloc with zero size.
 */
Test(simple, my_calloc_03)
{
	/* printf("my_calloc_03\n"); */
    void    *ptr = my_calloc(100, 0);
	cr_assert(ptr == NULL);
}

/**
 * @brief Test calloc and free.
 */
Test(simple, my_calloc_04)
{
	void    *ptr = my_calloc(100, 1);
	cr_assert(ptr != NULL);
	cr_assert(heapmetadata->flags == BUSY);
	my_free(ptr);
	cr_assert(heapmetadata->flags == FREE);
}

/* ***** End of simples tests calloc ***** */


/* ***** Begin of simples tests realloc ***** */

/**
 * @brief Test realloc to a larger size.
 */
Test(simple, my_realloc_01)
{
	void    *ptr = my_malloc(100);
	cr_assert(ptr != NULL);
	void    *ptr2 = my_realloc(ptr, 200);
	cr_assert(ptr2 != NULL);
}

/**
 * @brief Test realloc with an invalid pointer.
 */
Test(simple, my_realloc_02)
{
	void    *ptr = (void*)0xdeadbeef;
	void    *ptr2 = my_realloc(ptr, 200);
	cr_assert(ptr2 == NULL);
}

/**
 * @brief Test realloc with NULL pointer (equivalent to malloc).
 */
Test(simple, my_realloc_03)
{
	void    *ptr = NULL;
	void    *ptr2 = my_realloc(ptr, 10);
	cr_assert(ptr2 != NULL);
}

/**
 * @brief Test realloc to size 0 (equivalent to free).
 */
Test(simple, my_realloc_04)
{
	void    *ptr = my_malloc(100);
	cr_assert(ptr != NULL);
	void    *ptr2 = my_realloc(ptr, 0);
	cr_assert(ptr2 == NULL);
}

/**
 * @brief Test realloc with canary verification failure.
 */
Test(simple, my_realloc_05)
{
	void    *ptr = my_malloc(100);
	cr_assert(ptr != NULL);
	*((long *)((size_t)ptr + 100)) = 0xdeadbeef;
	void    *ptr2 = my_realloc(ptr, 10);
	cr_assert(ptr2 != NULL);
}

Test(simple, my_realloc_06)
{
	void    *ptr = my_malloc(1000);
	void 	*ptr2 = my_malloc(1000);
	void 	*ptr3 = my_malloc(1000);

	cr_assert(ptr != NULL);
	cr_assert(ptr2 != NULL);
	cr_assert(ptr3 != NULL);
	/* printf("(size_t)heapmetadata->next->addr = %ld\n", (size_t)heapmetadata->next->addr);  */
	/* printf("(size_t)heapmetadata + 1000+sizeof(long) = %ld\n", (size_t)heapmetadata->addr + 1000+sizeof(long)); */
	cr_assert((size_t)heapmetadata->next->addr == (size_t)heapmetadata->addr + 1000+sizeof(long));
	my_free(ptr2);

	void 	*ptr4 = my_realloc(ptr, 1500);
	
	cr_assert(ptr == ptr4);
	cr_assert(heapmetadata->size == 1500);

	printf("heapmetadata->next->size = %ld\n", heapmetadata->next->size);
	size_t global_size = (1000+sizeof(long))*3;
	size_t actual_size = heapmetadata->size + sizeof(long) + heapmetadata->next->size + sizeof(long) + heapmetadata->next->next->size + sizeof(long);
	printf("global_size = %ld\n", global_size);
	printf("actual_size = %ld\n", actual_size);
	cr_assert((void*)heapmetadata->next->addr == (void*)((size_t)heapmetadata->addr + 1500 + sizeof(long)));
	cr_assert(global_size == actual_size);
	cr_assert(heapmetadata->next->size == 500);
	cr_assert(heapmetadata->next->flags == FREE);
}

Test(simple, my_realloc_07)
{
	void *ptr = my_malloc(1000);
	void *ptr2 = my_malloc(1000);
	void *ptr3 = my_malloc(1000);
	void *ptr4 = my_malloc(1000);
	my_free(ptr2);
	my_free(ptr3);
	void *ptr5 = my_realloc(ptr, 1500);
	cr_assert(ptr == ptr5);
	cr_assert(heapmetadata->next->next->addr == ptr4);
}
/* ***** End of simples tests realloc ***** */
