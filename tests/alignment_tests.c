#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include "mymalloc_test.h"

#define ALIGN 16

int main(void) {
    const size_t cases[] = {
        1,
        ALIGN - 1,
        ALIGN,
        ALIGN + 1,
        4096,
        4096 + 1,
        (64 * 1024) - 1,
        (64 * 1024),
        (64 * 1024) + 1,
    };

    size_t ncases = sizeof(cases) / sizeof(cases[0]);
    for (size_t i = 0; i < ncases; i++) {
        size_t req = cases[i];

        // first free block returned from a new chunk
        struct Block *b = mymalloc_test_request_space(req);
        if (!b) {
            fprintf(stderr, "mymalloc_test_request_space(%zu) returned NULL\n", req);
            return 1;
        }

        // Place payload after block header
        uint8_t *payload = (uint8_t *)b + sizeof(*b);

        // Ensure payload is aligned.
        assert(((uintptr_t)payload & (ALIGN - 1)) == 0 && "payload not ALIGN-aligned");

        // usable payload should be at least the requested size
        size_t usable = b->size - sizeof(*b);
        if (usable < req) {
            fprintf(stderr, "FAIL: req=%zu usable=%zu\n", req, usable);
            return 1;
        }

        // check if writable inside requested space
        volatile uint8_t *p = payload;
        p[0] = 0xAA;
        p[req - 1] = 0xBB;

        printf("ok: req=%zu header=%p payload=%p usable=%zu\n",
               req, (void *)b, (void *)payload, usable);
    }

    puts("request_space alignment tests passed");
    return 0;
}
