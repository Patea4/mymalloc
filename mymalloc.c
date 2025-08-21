#include "mymalloc.h"
#include <sys/mman.h>

#define ALIGN 16
#define CHUNK_SIZE (64 * 1024)
#define MIN_BLOCK 16

typedef struct {
    int size;
} header_t;

static size_t g_pagesize = 0;

// Caches the pagesize for the system
size_t get_pagesize_cached() {
    if (g_pagesize == 0) {
        g_pagesize = sysconf(_SC_PAGESIZE);
        return g_pagesize;
    }
    return g_pagesize;
}

// Rounds up the number of pages needed for the request
// Also ensures that the minimum request size is satisfied.
size_t round_up_pages(size_t request_size, size_t pagesize) {

    if (request_size > pagesize) {
        return ((request_size + pagesize - 1) / pagesize) * pagesize;
    } else {
        return ((CHUNK_SIZE + pagesize - 1) / pagesize) * pagesize;
    }
}

void *os_alloc(size_t nbytes) {
    size_t pagesize = get_pagesize_cached();

    size_t request_size = round_up_pages(nbytes, pagesize);

    void *ptr = mmap(NULL, request_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    if (ptr == MAP_FAILED) {
        return NULL;
    }

    return ptr;
}

int os_release(void *ptr, size_t nbytes) {
    size_t pagesize = get_pagesize_cached();
    size_t size = round_up_pages(nbytes, pagesize);
    return munmap(ptr, size);
}

#ifdef MYMALLOC_TESTING
void *mymalloc_test_os_alloc(size_t n) { return os_alloc(n); }
int mymalloc_test_os_release(void *p, size_t n) { return os_release(p, n); }
size_t mymalloc_test_pagesize(void) { return get_pagesize_cached(); }
size_t mymalloc_test_roundup(size_t s, size_t pz) { return round_up_pages(s, pz); }
#endif
