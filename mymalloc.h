#ifndef MYMALLOC_H
#define MYMALLOC_H

#include <stddef.h>
#include <unistd.h>

void *malloc(size_t size);

void free(void *ptr);

void *realloc(void *ptr, size_t size);

void *calloc(size_t num, size_t size);

#endif
