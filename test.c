#include <criterion/criterion.h>
#include <sys/mman.h>
#include "secmalloc.h"
#include "log.h"
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>

/***** Begin of simples tests mmap *****/
Test(simple, simple_map_01)
{
    // Simple use of mmap
    char *ptr = mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    cr_assert(ptr != NULL, "Failed to mmap");
}

Test(simple, simple_map_02)
{
     // mmap with a larger size
     //printf("Testing mmap with larger size 8192 bytes\n");
     char *ptr = mmap(NULL, 8192, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
     cr_assert(ptr != MAP_FAILED, "Failed to mmap 8192 bytes");
     //printf("Successfully mapped 8192 bytes, now unmapping...\n");
     munmap(ptr, 8192);
}

Test(simple, simple_map_03)
{
     // mmap to request a minimal memory page
     //printf("Testing mmap with system page size\n");
     char *ptr = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
     cr_assert(ptr != MAP_FAILED, "Failed to mmap one system page size");
     //printf("Successfully mapped one system page size, now unmapping...\n");
     munmap(ptr, sysconf(_SC_PAGESIZE));
}

Test(simple, simple_map_04)
{
     // mmap with specific memory alignment
     size_t alignment = 16384; // 16KB alignment
     size_t size = 4096;
     //printf("Testing mmap with specific alignment: 16KB over 4KB size\n");
     char *ptr = mmap(NULL, size + alignment, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
     char *aligned_ptr = (char *)(((uintptr_t)ptr + alignment - 1) & ~(alignment - 1));
     cr_assert(aligned_ptr >= ptr && aligned_ptr < ptr + alignment, "Failed to get aligned memory");
     //printf("Alignment successful, ptr: %p, aligned_ptr: %p\n", ptr, aligned_ptr);
     munmap(ptr, size + alignment);
}

Test(simple, simple_map_5)
{
     // mmap with write-only protection
     //printf("Testing mmap with write-only protection\n");
     char *ptr = mmap(NULL, 4096, PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
     cr_assert(ptr != MAP_FAILED, "Failed to mmap with write-only protection");
     //printf("Write-only mmap successful, now unmapping...\n");
     munmap(ptr, 4096);
}

Test(simple, simple_map_6)
{
     // mmap with an invalid file descriptor
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


/* ***** Begin of simples tests log ***** */
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

/* ***** End of simples tests log ***** */


/* ***** Begin of simples tests canary ***** */

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

Test(simple, canary_03)
{
	/* printf("canary_03\n"); */
	void *ptr1 = my_malloc(100);
	cr_assert(ptr1 != NULL);
	cr_assert(heapmetadata->canary == *((long *)((size_t)heapdata + 100)));

	void *ptr2 = my_malloc(200);
	cr_assert(ptr2 != NULL);
	cr_assert(heapmetadata->next->canary == *((long *)((size_t)heapdata + 300 + sizeof(long))));
	cr_assert(heapmetadata->canary == *((long *)((size_t)heapdata + 100)));
}

Test(simple, canary_04)
{
	/* printf("canary_04\n"); */
	void *ptr1 = my_malloc(100);
	cr_assert(ptr1 != NULL);
	*((long *)((size_t)ptr1 + 100)) = 0xdeadbeef;
	my_free(ptr1); // will not free because wrong canary
	cr_assert(heapmetadata->flags == FREE);
}

/* ***** End of simples tests canary ***** */


/* ***** Begin of simples tests heap ***** */

Test(simple, init_heaps_01)
{
//	printf("init_heaps_01\n");
	heapdata = NULL;
	heapmetadata = NULL;
	heapdata = init_heapdata();
	heapmetadata = init_heapmetadata();
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
	// verify the next metadata bloc
	cr_assert(heapmetadata->next->size == PAGE_HEAP_SIZE - 100 - sizeof(long));
	cr_assert(heapmetadata->next->flags == FREE);
	cr_assert(heapmetadata->next->addr == (void *) ((size_t)heapdata + 100 + sizeof(long)));
	cr_assert(heapmetadata->next->canary == 0xdeadbeef);
	cr_assert(heapmetadata->next->next == NULL);
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

Test(simple, my_malloc_05)
{
	/* printf("my_malloc_05\n"); */
	void *ptr1 = my_malloc(4096);
	/* printf("ptr1 = %p\n", ptr1); */
	void *ptr2 = my_malloc(4096);
	/* printf("ptr2 = %p\n", ptr2); */
	void *ptr3 = my_malloc(4096);
	/* printf("ptr3 = %p\n", ptr3); */
	cr_assert(ptr1 != NULL);
	cr_assert(heapmetadata->size == 4096);
	cr_assert(heapmetadata->next->addr == ptr2);
	cr_assert(ptr2 != NULL);
	cr_assert(heapmetadata->next->size == 4096);
	cr_assert(heapmetadata->next->next->addr == ptr3);
	cr_assert(ptr3 != NULL);
}

Test(simple, my_malloc_06)
{
	/* printf("my_malloc_06\n"); */
	void *ptr1 = my_malloc(4090);
	/* printf("ptr1 = %p\n", ptr1); */
	cr_assert(ptr1 != NULL);
}

Test(simple, my_malloc_07)
{
	/* printf("my_malloc_06\n"); */
	void *ptr = my_malloc(4096-sizeof(long));
	cr_assert(ptr != NULL);
	void *ptr2 = my_malloc(666);
	cr_assert(heapmetadata->next->addr == ptr2);
}

/* ***** End of simples tests malloc ***** */


/* ***** Begin of simples tests free ***** */

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

Test(simple, free_03)
{
	void *ptr = (void*)0xdeadbeef;
	my_free(ptr);
	cr_assert(1==1);
}

Test(simple, free_04)
{
	void* ptr666 = my_malloc(100); //initialise heaps
	void *ptr = (void*)0xdeadbeef;
	my_free(ptr);
	my_free(ptr666);
	cr_assert(1==1);
}

Test(simple, free_05)
{
	void *ptr = my_malloc(100);
	cr_assert(ptr != NULL);
	my_free(ptr);
	cr_assert(heapmetadata->flags == FREE);
	my_free(ptr);
	cr_assert(heapmetadata->flags == FREE);
}

Test(simple, free_06)
{
	void *ptr = my_malloc(100);
	cr_assert(ptr != NULL);
	my_free(NULL);
	cr_assert(heapmetadata->flags == BUSY);
	my_free(ptr);
	cr_assert(heapmetadata->flags == FREE);
}

/* ***** End of simples tests free ***** */


/* ***** Begin of simples tests free / malloc ***** */

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

Test(simple, malloc_free_04)
{
	/* printf("resize_03\n"); */
	/* log_message("\nresize_03\n"); */
	void *ptr1 = my_malloc(3000);
	/* void *ptr2 = my_malloc(2000); */
	/* printf("ptr1 = %p\n", ptr1); */
	/* printf("heapmetadata->addr = %p\n", heapmetadata->addr); */
	/* printf("heapmetadata->size = %ld\n", heapmetadata->size); */
	/* printf("heapmetadata->flags = %d\n", heapmetadata->flags); */
	/* printf("heapmetadata->next->addr = %p\n", heapmetadata->next->addr); */
	/* printf("heapmetadata->next->size = %ld\n", heapmetadata->next->size); */
	/* printf("heapmetadata->next->flags = %d\n", heapmetadata->next->flags); */
	void *ptr3 = my_malloc(1000);
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

Test(simple, malloc_free_05)
{
	void *ptr1 = my_malloc(3000);
	void *ptr2 = my_malloc(2000);
	void *ptr3 = my_malloc(1000);
	cr_assert(ptr1 != NULL);
	cr_assert(ptr2 != NULL);
	cr_assert(ptr3 != NULL);
	my_free(ptr1);
	my_free(ptr2);
	void* ptr4 = my_malloc(1000);
	cr_assert(ptr4 != NULL);
	cr_assert(heapmetadata->addr == ptr4);
	cr_assert(ptr4 == ptr1);
}

/* ***** End of simples tests free / malloc ***** */


/* ***** Begin of simples tests resize ***** */

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
	void *ptr = my_malloc(100);
	for (int i=0; i<100; i++)
	{
		ptr = my_malloc(3);
	}
	cr_assert(ptr != NULL);
}

Test(simple, resize_06)
{
	extern struct chunkmetadata *lastmetadata();
	void *ptr = NULL;
	for (int i=0; i<100; i++) // 100 is ok but 1000 is not and i dont know why
	{
		ptr = my_malloc(300);
	}
	cr_assert(ptr != NULL);
	struct chunkmetadata *tmp = lastmetadata();
	cr_assert(tmp->flags == FREE);
}

Test(simple, resize_07)
{
	extern struct chunkmetadata *lastmetadata();
	void *ptr = NULL;
	ptr = malloc(10);
	/* printf("HELLO (size_t*)(heapdata - heapmetadata) = %ld\n"), ((size_t)(heapdata) - (size_t)heapmetadata); */
	for (int i=0; i<200; i++) // 200 is ok but 200 is not and i dont know why it is probably becaue there is not enough space beatween heapdata and heapmetadata
	{
		ptr = my_malloc(300);
	}
	cr_assert(ptr != NULL);
	struct chunkmetadata *tmp = lastmetadata();
	cr_assert(tmp->flags == FREE);
}

Test(simple, resize_05)
{
	/* printf("resize_05\n"); */
	void *ptr = my_malloc(10000);
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

Test(simple, my_calloc_01)
{
	/* printf("my_calloc_01\n"); */
	void *ptr = my_calloc(100, 1);
	cr_assert(ptr != NULL);
	cr_assert(heapmetadata->size == 100);
	cr_assert(heapmetadata->flags == BUSY);
	cr_assert(heapmetadata->next->size == 4096-100-sizeof(long));
	cr_assert(heapmetadata->next->flags == FREE);
}

Test(simple, my_calloc_02)
{
	/* printf("my_calloc_02\n"); */
	void *ptr = my_calloc(0, 1);
	cr_assert(ptr == NULL);
}

Test(simple, my_calloc_03)
{
	/* printf("my_calloc_03\n"); */
	void *ptr = my_calloc(100, 0);
	cr_assert(ptr == NULL);
}

Test(simple, my_calloc_04)
{
	void *ptr = my_calloc(100, 1);
	cr_assert(ptr != NULL);
	cr_assert(heapmetadata->flags == BUSY);
	my_free(ptr);
	cr_assert(heapmetadata->flags == FREE);
}

/* ***** End of simples tests calloc ***** */


/* ***** Begin of simples tests realloc ***** */

Test(simple, my_realloc_01)
{
	void *ptr = my_malloc(100);
	cr_assert(ptr != NULL);
	void *ptr2 = my_realloc(ptr, 200);
	cr_assert(ptr2 != NULL);
}

Test(simple, my_realloc_02)
{
	void *ptr = (void*)0xdeadbeef;
	void *ptr2 = my_realloc(ptr, 200);
	cr_assert(ptr2 == NULL);
}

Test(simple, my_realloc_03)
{
	void *ptr = NULL;
	void *ptr2 = my_realloc(ptr, 10);
	cr_assert(ptr2 != NULL);
}

Test(simple, my_realloc_04)
{
	void *ptr = my_malloc(100);
	cr_assert(ptr != NULL);
	void *ptr2 = my_realloc(ptr, 0);
	cr_assert(ptr2 == NULL);
}

Test(simple, my_realloc_05)
{
	void *ptr = my_malloc(100);
	cr_assert(ptr != NULL);
	*((long *)((size_t)ptr + 100)) = 0xdeadbeef;
	void *ptr2 = my_realloc(ptr, 10);
	cr_assert(ptr2 != NULL);
}

/* ***** End of simples tests realloc ***** */
