#include <criterion/criterion.h>
#include <sys/mman.h>
#include "my_alloc.private.h"
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>

/* #include "my_alloc.private.h" */

/* ***** Begin of simples tests mmap ***** */
Test(simple, simple_map_01)
{
	// utilisation simple d'un mmap
	char *ptr = mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	cr_assert(ptr != NULL, "Failed to mmap");
}

Test(simple, simple_map_02)
{
	// utilisation simple d'un mmap
	char *ptr1 = my_alloc(12);
	printf("simple_map_02\n");
	cr_assert(ptr1 != NULL, "Failed to alloc ptr1");
	char *ptr2 = my_alloc(25);
	cr_assert(ptr2 != NULL, "Failed to alloc ptr2");
	/* cr_assert(ptr1 != ptr2, "Failed to alloc ptr2 != ptr1"); */
	cr_assert((size_t)ptr2 == (size_t)ptr1 + 12 + (sizeof (struct chunk)), "Failed to alloc"); /* : %lx - %lx", (size_t)ptr2, (size_t)ptr1 + 12 + (sizeof (struct chunk)); */
	char *ptr3 = my_alloc(55);
	cr_assert((size_t)ptr3 == (size_t)ptr2 + 25 + (sizeof (struct chunk)), "Failed to alloc");/*  : %lx - %lx", (size_t)ptr3, (size_t)ptr2 + 25 + (sizeof (struct chunk))); */
}

Test(simple, simple_map_03)
{
	// utilisation simple d'un mmap
	char *ptr1 = my_alloc(12);
	printf("simple_map_02\n");
	cr_assert(ptr1 != NULL, "Failed to alloc ptr1");
	char *ptr2 = my_alloc(25);
	cr_assert(ptr2 != NULL, "Failed to alloc ptr2");
	/* cr_assert(ptr1 != ptr2, "Failed to alloc ptr2 != ptr1"); */
	cr_assert((size_t)ptr2 == (size_t)ptr1 + 12 + (sizeof (struct chunk)), "Failed to alloc"); /* : %lx - %lx", (size_t)ptr2, (size_t)ptr1 + 12 + (sizeof (struct chunk)); */
	char *ptr3 = my_alloc(55);
	cr_assert((size_t)ptr3 == (size_t)ptr2 + 25 + (sizeof (struct chunk)), "Failed to alloc");/*  : %lx - %lx", (size_t)ptr3, (size_t)ptr2 + 25 + (sizeof (struct chunk))); */
	printf("clean ptr1\n");
    clean(ptr1);
    printf("clean ptr2\n");
    clean(ptr2);
    struct chunk *t = (struct chunk *)((size_t)ptr1 - sizeof (struct chunk));
    printf("t : %lu\n", t->size);
    cr_assert(t->size == 12 + 25 + sizeof(struct chunk), "Failed to clean");
}

Test(simple, simple_map_04)
{
	// utilisation simple d'un mmap
	char *ptr1 = my_alloc(12);
	cr_assert(ptr1 != NULL, "Failed to alloc ptr1");
	char *ptr2 = my_alloc(25);
	cr_assert(ptr2 != NULL, "Failed to alloc ptr2");
	cr_assert((size_t)ptr2 == (size_t)ptr1 + 12 + (sizeof (struct chunk)), "Failed to alloc"); /* : %lx - %lx", (size_t)ptr2, (size_t)ptr1 + 12 + (sizeof (struct chunk)); */
	char *ptr3 = my_alloc(55);
	cr_assert(ptr3 != NULL, "Failed to alloc ptr3");
	cr_assert((size_t)ptr3 == (size_t)ptr2 + 25 + (sizeof (struct chunk)), "Failed to alloc");/*  : %lx - %lx", (size_t)ptr3, (size_t)ptr2 + 25 + (sizeof (struct chunk))); */
	printf("clean ptr2\n");
	clean(ptr2);
	printf("clean ptr3\n");
	clean(ptr3);
	struct chunk *t = (struct chunk*)((size_t)ptr2 - sizeof(struct chunk));
	printf("t->size = %ld\n", t->size);
	cr_assert(t->size == 25 + 55 + 3940 + 2 * sizeof(struct chunk), "Failed to clean");
}

Test(simple, simple_map_05)
{
	// utilisation simple d'un mmap
	char *ptr1 = my_alloc(12);
	cr_assert(ptr1 != NULL, "Failed to alloc ptr1");
	char *ptr2 = my_alloc(25);
	cr_assert(ptr2 != NULL, "Failed to alloc ptr2");
	cr_assert((size_t)ptr2 == (size_t)ptr1 + 12 + (sizeof (struct chunk)), "Failed to alloc"); /* : %lx - %lx", (size_t)ptr2, (size_t)ptr1 + 12 + (sizeof (struct chunk)); */
	char *ptr3 = my_alloc(55);
	cr_assert((size_t)ptr3 == (size_t)ptr2 + 25 + (sizeof (struct chunk)), "Failed to alloc");/*  : %lx - %lx", (size_t)ptr3, (size_t)ptr2 + 25 + (sizeof (struct chunk))); */
	printf("clean ptr1\n");
	clean(ptr1);
	printf("clean ptr2\n");
	clean(ptr2);
	struct chunk *t = (struct chunk*)((size_t)ptr1 - sizeof(struct chunk));
	printf("t->size = %ld\n", t->size);
	cr_assert(t->size == 12 + 25 + sizeof(struct chunk), "Failed to clean ptr2");
}

Test(simple, simple_map_06)
{
	char *ptr1 = my_alloc(8192);
	cr_assert(ptr1 != NULL, "Failed to alloc ptr1");
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
    struct chunk *result = lookup(100);  // Searches for a 100-byte block in an initialized empty heap
    cr_assert_not_null(result, "No block found in an initialized but empty heap.");
    cr_assert(result->size >= 100, "The found block is not large enough.");
    cr_assert(result->flags == FREE, "The found block is not free.");
}

// Test the lookup function after a block has been allocated
Test(lookup_tests, lookup_after_allocation)
{
    init_heap();  // Reinitializes the heap for this test
    void *ptr = my_alloc(200);  // Allocates a 200-byte block
    struct chunk *result = lookup(100);  // Searches for another 100-byte block
    cr_assert_not_null(result, "No block found after an allocation.");
    cr_assert(result->size >= 100, "The found block is not large enough after an allocation.");
    cr_assert(result->flags == FREE, "The found block is not free after an allocation.");
    clean(ptr);  // Cleans up to prevent memory leaks
}

// Test the lookup function when the heap is fully utilized
Test(lookup_tests, lookup_full_heap)
{
    init_heap();  // Reinitializes the heap for this test
    my_alloc(heap_size - sizeof(struct chunk));  // Attempts to allocate almost the entire heap
    struct chunk *result = lookup(100);  // Searches for a 100-byte block
    cr_assert_null(result, "A block was found while the heap should be full.");
}

// Test the lookup function for a size that does not exist in the heap
Test(lookup_tests, lookup_non_existent_size)
{
    init_heap();  // Reinitializes the heap for this test
    struct chunk *result = lookup(heap_size * 2);  // Searches for a block larger than the heap itself
    cr_assert_null(result, "A block was found even though no block of this size should exist.");
}

/* ***** End of simples tests lookup ***** */
