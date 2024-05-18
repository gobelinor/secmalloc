#include <criterion/criterion.h>
#include <sys/mman.h>
#include "my_alloc.private.h"
#include "log.h"
#include <stdio.h>

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
