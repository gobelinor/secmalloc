#include <criterion/criterion.h>
#include <sys/mman.h>
#include "secmalloc.private.h"
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
    printf("Testing mmap with larger size 8192 bytes\n");
    char *ptr = mmap(NULL, 8192, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    cr_assert(ptr != MAP_FAILED, "Failed to mmap 8192 bytes");
    printf("Successfully mapped 8192 bytes, now unmapping...\n");
    munmap(ptr, 8192);
}

Test(simple, simple_map_08)
{
    // mmap pour demander une page mémoire minimale
    printf("Testing mmap with system page size\n");
    char *ptr = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    cr_assert(ptr != MAP_FAILED, "Failed to mmap one system page size");
    printf("Successfully mapped one system page size, now unmapping...\n");
    munmap(ptr, sysconf(_SC_PAGESIZE));
}

Test(simple, simple_map_09)
{
    // mmap avec un alignement de mémoire spécifique
    size_t alignment = 16384; // 16KB alignment
    size_t size = 4096;
    printf("Testing mmap with specific alignment: 16KB over 4KB size\n");
    char *ptr = mmap(NULL, size + alignment, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    char *aligned_ptr = (char *)(((uintptr_t)ptr + alignment - 1) & ~(alignment - 1));
    cr_assert(aligned_ptr >= ptr && aligned_ptr < ptr + alignment, "Failed to get aligned memory");
    printf("Alignment successful, ptr: %p, aligned_ptr: %p\n", ptr, aligned_ptr);
    munmap(ptr, size + alignment);
}

Test(simple, simple_map_10)
{
    // mmap avec protection en écriture uniquement
    printf("Testing mmap with write-only protection\n");
    char *ptr = mmap(NULL, 4096, PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    cr_assert(ptr != MAP_FAILED, "Failed to mmap with write-only protection");
    printf("Write-only mmap successful, now unmapping...\n");
    munmap(ptr, 4096);
}

Test(simple, simple_map_11)
{
    // mmap avec un descripteur de fichier invalide
    printf("Testing mmap with an invalid file descriptor\n");
    char *ptr = mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_PRIVATE, -1, 0);
    cr_assert(ptr == MAP_FAILED, "mmap should fail with an invalid file descriptor");
    if (ptr != MAP_FAILED) {
        printf("Unexpected success, now unmapping...\n");
        munmap(ptr, 4096);
    } else {
        printf("Failed as expected with invalid file descriptor\n");
    }
}

/* ***** End of simples tests mmap ***** */

/* ***** Begin of simples tests lookup ***** */
Test(lookup_tests, lookup_empty_heap)
{
    init_heap();  // Initializes the heap
    struct chunkmetadata *result = lookup(100);  // Searches for a 100-byte block in an initialized empty heap
    cr_assert_not_null(result, "No block found in an initialized but empty heap.");
    cr_assert(result->size >= 100, "The found block is not large enough.");
    cr_assert(result->flags == FREE, "The found block is not free.");
}

Test(lookup_tests, lookup_after_allocation)
{
    init_heap();  // Reinitializes the heap for this test
    void *ptr = my_malloc(200);  // Allocates a 200-byte block
    struct chunkmetadata *result = lookup(100);  // Searches for another 100-byte block
    cr_assert_not_null(result, "No block found after an allocation.");
    cr_assert(result->size >= 100, "The found block is not large enough after an allocation.");
    cr_assert(result->flags == FREE, "The found block is not free after an allocation.");
    clean(ptr);  // Cleans up to prevent memory leaks
}

/* ***** End of simples tests lookup ***** */


/* ***** Begin of simples tests log ***** */
Test(simple, log_01)
{
	printf("log_01\n");
	int ret = log_new_execution();
	printf("ret = %d\n", ret);
	cr_assert(ret == 0);
}

Test(simple, log_02)
{
	printf("log_02\n");
	int ret = log_message("ecrit ce que tu %s : %d\n", "veux", 12);
	printf("ret = %d\n", ret);
	cr_assert(ret == 0);
}

Test(simple, log_03)
{	
	printf("log_03\n");
	void *ptr = my_malloc(100);
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
	printf("canary_01\n");
	long ret = generate_canary();
	printf("canary = %ld\n", ret);
	printf("sizeofcanary = %ld\n", sizeof(long));
	printf("sizeofaddr = %ld\n", sizeof(void *));
	cr_assert(ret != -1);
	cr_assert(sizeof(long) == sizeof(void *));
}

Test(simple, canary_02) // not perfect but that should be enough for our purpuses
{
	printf("canary_02\n");
	for (int i = 0; i < 100; i++)
	{	
		long ret1 = generate_canary();
		cr_assert(ret1 != -1);		
		long ret2 = generate_canary();
		cr_assert(ret2 != -1);
		cr_assert(ret1 != ret2);
	}
}

/* ***** End of simples tests canary ***** */


/* ***** Begin of simples tests heap ***** */

Test(simple, init_heaps_01)
{
	printf("init_heaps_01\n");
	void *heapdata = NULL;
	struct chunkmetadata *heapmetadata = NULL;
	size_t heap_size = 4096;
	heapdata = init_heapdata();
	heapmetadata = init_heapmetadata();
	cr_assert(heapmetadata != NULL);
	cr_assert(heapdata != NULL);
	cr_assert(heapmetadata->size == heap_size-sizeof(struct chunkmetadata));
	cr_assert(heapmetadata->flags == FREE);
	cr_assert(heapmetadata->addr == heapdata);
	cr_assert(heapmetadata->canary == 0xdeadbeef);
	cr_assert(heapmetadata->next == NULL);
	cr_assert(heapmetadata->prev == NULL);
}

/* ***** End of simples tests heap ***** */
