#include <stddef.h>

#define TEST_CHUNK_SIZE (64 * 1024)

typedef struct Block {
    size_t size;
    struct Block *next;
    int free;
} Block;

void *mymalloc_test_os_alloc(size_t nbytes);

int mymalloc_test_os_release(void *p, size_t n);

size_t mymalloc_test_pagesize(void);

size_t mymalloc_test_roundup(size_t s, size_t pz);

Block *mymalloc_test_request_space(size_t size);
