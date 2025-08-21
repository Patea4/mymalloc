#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "mymalloc_test.h"

static size_t round_up_pages(size_t n, size_t ps) {
    return ((n + ps - 1) / ps) * ps;
}

int main(int argc, char *argv[]) {

    size_t ps = mymalloc_test_pagesize();
    const size_t cases[] = {
        1,
        ps - 1,
        ps,
        ps + 1,
        (2 * ps) + 100,
        (64 * 1024) - 1,
        (64 * 1024),
        (64 * 1024) + 1,
    };

    for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
        size_t requested = cases[i];
        size_t expected = round_up_pages(requested, ps);

        void *p = mymalloc_test_os_alloc(requested);
        if (p == NULL) {
            perror("osalloc");
            exit(1);
        }

        // Write to every byte to check they have been allocated correctly
        volatile uint8_t *data = (volatile uint8_t *)p;
        for (size_t j = 0; j < expected; j++) {
            data[j] = 0x11;
        }
        data[expected - 1] = 0x22;

        printf("ok: request=%zu, expected=%zu, wrote to every byte\n", requested, expected);
    }
}
