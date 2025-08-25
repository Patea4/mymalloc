#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "mymalloc_test.h"

static int is_aligned(void *p, size_t a) {
    return ((uintptr_t)p % a) == 0;
}

int main(void) {
    mymalloc_test_reset();

    size_t sizes[] = {1, 15, 16, 17, 31, 32, 33, 4096, 4097, 65535, 65536};
    void *ptrs[sizeof(sizes) / sizeof(sizes[0])] = {0};

    for (size_t i = 0; i < sizeof(sizes) / sizeof(sizes[0]); i++) {
        ptrs[i] = malloc(sizes[i]);
        assert(ptrs[i] != NULL);
        assert(is_aligned(ptrs[i], ALIGN));
    }

    // Free them all
    for (size_t i = 0; i < sizeof(sizes) / sizeof(sizes[0]); i++) {
        free(ptrs[i]);
    }

    puts("ok: alignment tests");
    return 0;
}
