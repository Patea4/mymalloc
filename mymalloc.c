#include "mymalloc.h"
#include <stdint.h>
#include <unistd.h>
#include <sys/mman.h>

#define ALIGN 16
#define CHUNK_SIZE (64 * 1024)
#define MIN_BLOCK 16

/*
 * Represents a chunk of memory given by the OS
 * Each Chunk stores its own size and pointer to the next Chunk.
 *
 * A chunk is made of various Blocks of memory.
 */
typedef struct Chunk {
    size_t size;
    struct Chunk *next;
} Chunk;

/*
 * Represents a free memory block in the heap.
 * Each Block stores its own size and pointer to the next free block
 * The Size includes the header of the block.
 */
typedef struct Block {
    size_t size; // Total size of block including header.
    struct Block *next;
    int free; // 1 if true, 0 if false
} Block;

static size_t g_pagesize = 0;
static Block *g_free_head = NULL;
static Chunk *g_chunks = NULL;

// Forward Declarations
static size_t get_pagesize_cached();
static size_t round_up(size_t request_size, size_t pagesize);
static void *os_alloc(size_t nbytes);
static int os_release(void *ptr, size_t nbytes);

// Aligns pointer to the nearest 16 byte multiple
static uintptr_t align_ptr(uintptr_t ptr) {
    return (ptr + ALIGN - 1) & ~(uintptr_t)(ALIGN - 1);
}

static Block *request_space(size_t size) {
    size_t pagesize = get_pagesize_cached();

    size_t total = sizeof(Chunk) + sizeof(Block) + size;
    total = round_up(total, pagesize);

    // Request enough size for freelist and requested size
    void *ptr = os_alloc(total);

    // Store the Chunk data
    Chunk *chunk = (Chunk *)ptr; // 0x0000
    chunk->size = total;
    chunk->next = g_chunks;
    g_chunks = chunk; // 0x0010

    // Find position after chunk header, then reserve space for
    // block header. Then align the end of block head to start payload.
    uintptr_t after_header = (uintptr_t)(chunk + 1);
    uintptr_t aligned_block = align_ptr(after_header + sizeof(Block));

    size_t padding = (size_t)(aligned_block - (uintptr_t)after_header);

    Block *b = (Block *)(aligned_block - sizeof(Block));
    b->size = total - sizeof(Chunk) - padding;
    b->free = 1;
    b->next = NULL;

    return b;
}

void *malloc(size_t size) {
    if (size <= 0) {
        return NULL;
    }

    // Heap has not been initialized
    if (!g_free_head) {
    }

    return NULL;
}

/*
 * Caches the pagesize for the system
 *
 * Returns:
 *  The size of the system's pagesize.
 */
static size_t get_pagesize_cached() {
    if (g_pagesize == 0) {
        g_pagesize = sysconf(_SC_PAGESIZE);
        return g_pagesize;
    }
    return g_pagesize;
}

/*
 * Rounds up the requested memory to be a multiple of align,
 * Used for aligning to pages or to ALIGN
 * Args:
 *  request_size: The number of bytes to be rounded up.
 *  align: What to align against to
 * Returns:
 *  The requested size rounded up to the closest align multiple.
 */
static size_t round_up(size_t request_size, size_t align) {

    if (request_size > CHUNK_SIZE) {
        return ((request_size + align - 1) / align) * align;
    } else {
        return ((CHUNK_SIZE + align - 1) / align) * align;
    }
}

/*
 * Requests and allocates the amount of memory requested
 * from the OS. Note that the memory requested will be
 * rounded up to a multiple of the OS' pagesize.
 *
 * Args:
 *  nbytes: The minimum number of bytes to request
 *
 * Returns:
 *  A pointer to the allocated memory on success.
 *  NULL if allocation fails, and errno will be set.
 */
static void *os_alloc(size_t nbytes) {
    size_t pagesize = get_pagesize_cached();

    size_t request_size = round_up(nbytes, pagesize);

    void *ptr = mmap(
        NULL,
        request_size,
        PROT_READ | PROT_WRITE,
        MAP_PRIVATE | MAP_ANONYMOUS,
        -1,
        0);

    if (ptr == MAP_FAILED) {
        return NULL;
    }

    return ptr;
}

/*
 * Deallocates memory pointed by the passed pointer
 * until the given size has been reached.
 *
 * Returns:
 *  0 if successful
 *  -1 if unsuccessful, and errno will be set.
 */
static int os_release(void *ptr, size_t nbytes) {
    size_t pagesize = get_pagesize_cached();
    size_t size = round_up(nbytes, pagesize);
    return munmap(ptr, size);
}

// Helper functions to be defined when testing.
#ifdef MYMALLOC_TESTING
void *mymalloc_test_os_alloc(size_t n) { return os_alloc(n); }
int mymalloc_test_os_release(void *p, size_t n) { return os_release(p, n); }
size_t mymalloc_test_pagesize(void) { return get_pagesize_cached(); }
size_t mymalloc_test_roundup(size_t s, size_t pz) { return round_up(s, pz); }
Block *mymalloc_test_request_space(size_t size) { return request_space(size); }
#endif
