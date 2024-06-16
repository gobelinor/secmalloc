#ifndef SECMALLOC_H
#define SECMALLOC_H

#include <stddef.h>
#include "secmalloc.private.h"

/**
 * @file secmalloc.h
 * @brief Header file for secure memory allocation functions.
 *
 * This file contains the declarations for secure memory allocation
 * functions used throughout the project.
 */

/**
 * @brief Allocates memory securely.
 *
 * This function allocates a block of memory of the specified size.
 *
 * @param size The size of the memory block to allocate.
 * @return void* A pointer to the allocated memory, or NULL if the allocation fails.
 */
void    *my_malloc(size_t size);

/**
 * @brief Frees allocated memory.
 *
 * This function frees a previously allocated block of memory.
 *
 * @param ptr A pointer to the memory block to free.
 */
void    my_free(void* ptr);

/**
 * @brief Allocates memory for an array securely.
 *
 * This function allocates memory for an array of elements and initializes all its bits to zero.
 *
 * @param nmemb The number of elements.
 * @param size The size of each element.
 * @return void* A pointer to the allocated memory, or NULL if the allocation fails.
 */
void    *my_calloc(size_t nmemb, size_t size);

/**
 * @brief Reallocates memory securely.
 *
 * This function changes the size of a previously allocated memory block.
 *
 * @param ptr A pointer to the previously allocated memory block.
 * @param size The new size of the memory block.
 * @return void* A pointer to the reallocated memory, or NULL if the reallocation fails.
 */
void    *my_realloc(void* ptr, size_t size);

/**
 * @brief Allocates memory.
 *
 * This function allocates a block of memory of the specified size.
 *
 * @param size The size of the memory block to allocate.
 * @return void* A pointer to the allocated memory, or NULL if the allocation fails.
 */
void    *malloc(size_t size);

/**
 * @brief Frees allocated memory.
 *
 * This function frees a previously allocated block of memory.
 *
 * @param ptr A pointer to the memory block to free.
 */
void    free(void* ptr);

/**
 * @brief Allocates memory for an array.
 *
 * This function allocates memory for an array of elements and initializes all its bits to zero.
 *
 * @param nmemb The number of elements.
 * @param size The size of each element.
 * @return void* A pointer to the allocated memory, or NULL if the allocation fails.
 */
void    *calloc(size_t nmemb, size_t size);

/**
 * @brief Reallocates memory.
 *
 * This function changes the size of a previously allocated memory block.
 *
 * @param ptr A pointer to the previously allocated memory block.
 * @param size The new size of the memory block.
 * @return void* A pointer to the reallocated memory, or NULL if the reallocation fails.
 */
void    *realloc(void* ptr, size_t size);

#endif // SECMALLOC_H
