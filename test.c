#include <criterion/criterion.h>
#include <sys/mman.h>
#include "secmalloc.h"
#include "log.h"
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>

/* ***** Begin of simples tests mmap ***** */
Test(simple, simple_map_01)
{
	// utilisation simple d'un mmap
	char *ptr = mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	cr_assert(ptr != NULL, "Failed to mmap");
}

Test(simple, simple_map_07)
{
    // mmap avec une taille plus grande
    //printf("Testing mmap with larger size 8192 bytes\n");
    char *ptr = mmap(NULL, 8192, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    cr_assert(ptr != MAP_FAILED, "Failed to mmap 8192 bytes");
    //printf("Successfully mapped 8192 bytes, now unmapping...\n");
    munmap(ptr, 8192);
}

Test(simple, simple_map_08)
{
    // mmap pour demander une page mémoire minimale
    //printf("Testing mmap with system page size\n");
    char *ptr = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    cr_assert(ptr != MAP_FAILED, "Failed to mmap one system page size");
    //printf("Successfully mapped one system page size, now unmapping...\n");
    munmap(ptr, sysconf(_SC_PAGESIZE));
}

Test(simple, simple_map_09)
{
    // mmap avec un alignement de mémoire spécifique
    size_t alignment = 16384; // 16KB alignment
    size_t size = 4096;
    //printf("Testing mmap with specific alignment: 16KB over 4KB size\n");
    char *ptr = mmap(NULL, size + alignment, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    char *aligned_ptr = (char *)(((uintptr_t)ptr + alignment - 1) & ~(alignment - 1));
    cr_assert(aligned_ptr >= ptr && aligned_ptr < ptr + alignment, "Failed to get aligned memory");
    //printf("Alignment successful, ptr: %p, aligned_ptr: %p\n", ptr, aligned_ptr);
    munmap(ptr, size + alignment);
}

Test(simple, simple_map_10)
{
    // mmap avec protection en écriture uniquement
    //printf("Testing mmap with write-only protection\n");
    char *ptr = mmap(NULL, 4096, PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    cr_assert(ptr != MAP_FAILED, "Failed to mmap with write-only protection");
    //printf("Write-only mmap successful, now unmapping...\n");
    munmap(ptr, 4096);
}

Test(simple, simple_map_11)
{
    // mmap avec un descripteur de fichier invalide
    //printf("Testing mmap with an invalid file descriptor\n");
    char *ptr = mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_PRIVATE, -1, 0);
    cr_assert(ptr == MAP_FAILED, "mmap should fail with an invalid file descriptor");
    if (ptr != MAP_FAILED) {
      //  printf("Unexpected success, now unmapping...\n");
        munmap(ptr, 4096);
    } else {
        //printf("Failed as expected with invalid file descriptor\n");
    }
}

/* ***** End of simples tests mmap ***** */

/* ***** Begin of simples tests lookup ***** */
/*  */
/* Test(lookup_tests, lookup_empty_heap) */
/* { */
/*     init_heap();  // Initializes the heap */
/*     struct chunk *result = lookup(100);  // Searches for a 100-byte block in an initialized empty heap */
/*     cr_assert_not_null(result, "No block found in an initialized but empty heap."); */
/*     cr_assert(result->size >= 100, "The found block is not large enough."); */
/*     cr_assert(result->flags == FREE, "The found block is not free."); */
/* } */
/*  */
/* // Test the lookup function after a block has been allocated */
/* Test(lookup_tests, lookup_after_allocation) */
/* { */
/*     init_heap();  // Reinitializes the heap for this test */
/*     void *ptr = my_alloc(200);  // Allocates a 200-byte block */
/*     struct chunk *result = lookup(100);  // Searches for another 100-byte block */
/*     cr_assert_not_null(result, "No block found after an allocation."); */
/*     cr_assert(result->size >= 100, "The found block is not large enough after an allocation."); */
/*     cr_assert(result->flags == FREE, "The found block is not free after an allocation."); */
/*     clean(ptr);  // Cleans up to prevent memory leaks */
/* } */
/*  */
/* // Test the lookup function when the heap is fully utilized */
/* Test(lookup_tests, lookup_full_heap) */
/* { */
/*     init_heap();  // Reinitializes the heap for this test */
/*     my_alloc(pageheap_size - sizeof(struct chunk));  // Attempts to allocate almost the entire heap */
/*     struct chunk *result = lookup(100);  // Searches for a 100-byte block */
/*     cr_assert_null(result, "A block was found while the heap should be full."); */
/* } */
/*  */
/* // Test the lookup function for a size that does not exist in the heap */
/* Test(lookup_tests, lookup_non_existent_size) */
/* { */
/*     init_heap();  // Reinitializes the heap for this test */
/*     struct chunk *result = lookup(pageheap_size * 2);  // Searches for a block larger than the heap itself */
/*     cr_assert_null(result, "A block was found even though no block of this size should exist."); */
/* } */
/*  */
/* ***** End of simples tests lookup ***** */





Test(simple, log_01)
{
//	printf("log_01\n");
	int ret = log_new_execution();
//	printf("ret = %d\n", ret);
	cr_assert(ret == 0);
}

Test(simple, log_02)
{
//	printf("log_02\n");
	int ret = log_message("ecrit ce que tu %s : %d\n", "veux", 12);
//	printf("ret = %d\n", ret);
	cr_assert(ret == 0);
}

Test(simple, log_03)
{	
//	printf("log_03\n");
	void *ptr = malloc(100);
	int ret = log_event(MALLOC, START, NULL, 100);
	cr_assert(ret == 0);
    ret = log_event(MALLOC, END, ptr, 100); 
	cr_assert(ret == 0);
    ret = log_event(FREE_fn, START, ptr, 100);
	cr_assert(ret == 0);
    ret = log_event(FREE_fn, END, ptr, 100); 
	cr_assert(ret == 0);
	free(ptr);
}

Test(simple, canary_01)
{
//	printf("canary_01\n");
	long ret = generate_canary();
//	printf("canary = %ld\n", ret);
//	printf("sizeofcanary = %ld\n", sizeof(long));
//	printf("sizeofaddr = %ld\n", sizeof(void *));
	cr_assert(ret != -1);
	cr_assert(sizeof(long) == sizeof(void *));
}

Test(simple, canary_02) // not perfect but that should be enough for our purpuses
{
//	printf("canary_02\n");
	for (int i = 0; i < 100; i++)
	{	
		long ret1 = generate_canary();
		cr_assert(ret1 != -1);		
		long ret2 = generate_canary();
		cr_assert(ret2 != -1);
		cr_assert(ret1 != ret2);
	}
}

Test(simple, init_heaps_01)
{
//	printf("init_heaps_01\n");
	heapdata = NULL;
	heapmetadata = NULL;
	heapdata = init_heapdata();
	heapmetadata = init_heapmetadata();
	cr_assert(heapmetadata != NULL);
	cr_assert(heapdata != NULL);
	cr_assert(heapmetadata->size == pageheap_size);
	cr_assert(heapmetadata->flags == FREE);
	cr_assert(heapmetadata->addr == heapdata);
	cr_assert(heapmetadata->canary == 0xdeadbeef);
	cr_assert(heapmetadata->next == NULL);
	cr_assert(heapmetadata->prev == NULL);
}

Test(simple, my_malloc_01)
{
	/* printf("my_malloc_01\n"); */
	void *ptr = my_malloc(100);
	cr_assert(ptr != NULL);
	cr_assert(ptr == heapdata);
	// verify the first metadata bloc (heapmetadata)
	cr_assert(heapmetadata->addr == ptr);
	cr_assert(heapmetadata->size == 100);
	cr_assert(heapmetadata->flags == BUSY);
	long canary = *((long *)((size_t)ptr + 100));
	cr_assert(heapmetadata->canary == canary);
	/* printf("heapmetadata->next = %p\n", heapmetadata->next); */
	/* printf("heapmetadata = %p\n", heapmetadata); */
	/* printf("sizeof(struct chunkmetadata) = %ld\n", sizeof(struct chunkmetadata)); */
	/* printf("heapmetadata + sizeof(struct chunkmetadata) = %p\n", (void*)((size_t)heapmetadata + sizeof(struct chunkmetadata))); */
	cr_assert(heapmetadata->next == (void *) ((size_t)heapmetadata + sizeof(struct chunkmetadata)));
	cr_assert(heapmetadata->prev == NULL);
	// verify the next metadata bloc
	cr_assert(heapmetadata->next->size == pageheap_size - 100);
	cr_assert(heapmetadata->next->flags == FREE);
	cr_assert(heapmetadata->next->addr == (void *) ((size_t)heapdata + 100));	
	cr_assert(heapmetadata->next->canary == 0xdeadbeef);
	cr_assert(heapmetadata->next->next == NULL);
	cr_assert(heapmetadata->next->prev == heapmetadata);
}

Test(simple, my_malloc_02)
{
//	printf("my_malloc_02\n");
	void *ptr = my_malloc(0);
	cr_assert(ptr == NULL);
//	printf("ptr = %p\n", ptr);
}

Test(simple, my_malloc_03)
{
	heapdata = NULL;
	heapmetadata = NULL;
	/* printf("my_malloc_03\n"); */
	/* printf("heapdata = %p\n", heapdata); */
	/* printf("metadataheap = %p\n", heapmetadata); */
	void *ptr1 = my_malloc(100);
	/* printf("metadataheap = %p\n", heapmetadata); */
	/* printf("heapdata = %p\n", heapdata); */
	/* printf("ptr1 = %p\n", ptr1); */
	cr_assert(ptr1 != NULL);
	void *ptr2 = my_malloc(100);
	/* printf("ptr2 = %p\n", ptr2); */
	cr_assert(ptr2 != NULL);
	cr_assert(ptr1 != ptr2);
}

Test(simple, my_malloc_04)
{
	void *ptr = my_malloc(1000);
	cr_assert(ptr != NULL);
	long canary = *((long *)((size_t)ptr + 1000));
	cr_assert(heapmetadata->canary == canary);
}

Test(simple, free_01)
{
	void *ptr = my_malloc(100);
	cr_assert(ptr != NULL);
	my_free(ptr);
	cr_assert(heapmetadata->flags == FREE);	
}

Test(simple, free_02)
{
	void *ptr = my_malloc(100);
	cr_assert(ptr != NULL);
	my_free(ptr);
	cr_assert(heapmetadata->next == NULL);
}

Test(simple, malloc_free_01)
{
	void *ptr = my_malloc(100);
	cr_assert(ptr != NULL);
	my_free(ptr);
	void *ptr2 = my_malloc(100);
	cr_assert(ptr2 != NULL);
	cr_assert(ptr == ptr2);
}

Test(simple, malloc_free_02)
{
	void *ptr = my_malloc(100);
	cr_assert(ptr != NULL);
	my_free(ptr);
	void *ptr2 = my_malloc(100);
	cr_assert(ptr2 != NULL);
	my_free(ptr2);
}

Test(simple, malloc_free_03)
{
	void *ptr = my_malloc(100);
	cr_assert(ptr != NULL);
	void *ptr2 = my_malloc(100);
	cr_assert(ptr2 != NULL);
	void *ptr3 = my_malloc(100);
	my_free(ptr);
	my_free(ptr2);
	cr_assert(heapmetadata->next->addr == ptr3);
	my_free(ptr3);
}


Test(simple, resize_01)
{
	/* printf("resize_01\n"); */
	void *ptr = my_malloc(5000);
	/* printf("ptr = %p\n", ptr); */
	cr_assert(ptr != NULL);
}

Test(simple, resize_02)
{
	/* printf("resize_02\n"); */
	void *ptr = my_malloc(5000);
	/* printf("ptr = %p\n", ptr); */
	cr_assert(ptr != NULL);
	my_free(ptr);
}

Test(simple, resize_03)
{
	/* printf("resize_03\n"); */
	void *ptr1 = my_malloc(3000);
	/* void *ptr2 = my_malloc(2000); */
	printf("ptr1 = %p\n", ptr1);
	printf("heapmetadata->addr = %p\n", heapmetadata->addr);
	printf("heapmetadata->size = %ld\n", heapmetadata->size);
	void *ptr3 = my_malloc(1000);
	printf("ptr3 = %p\n", ptr3);
	printf("heapmetadata->next->addr = %p\n", heapmetadata->next->addr);
	printf("heapmetadata->next->size = %ld\n", heapmetadata->next->size);
	cr_assert(ptr1 != NULL);
	/* cr_assert(ptr2 != NULL); */
	cr_assert(ptr3 != NULL);
	my_free(ptr1);
	printf("post free\n");
	/* my_free(ptr2); */
	printf("heapmetadata->next->addr = %p\n", heapmetadata->next->addr);
	cr_assert(heapmetadata->addr == ptr1);	
	cr_assert(heapmetadata->next->addr == ptr3);
	my_free(ptr3);
}

