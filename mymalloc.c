#include "mymalloc.h"
#include <stdint.h>
#include <unistd.h>
#include <sys/mman.h>
#include <string.h>

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
} Block;

#define HDRSIZE ((size_t)((sizeof(Block) + (ALIGN - 1)) & ~(ALIGN - 1)))

static size_t g_pagesize = 0;
static Block *g_free_head = NULL;
static Chunk *g_chunks = NULL;

// Forward Declarations
static size_t get_pagesize_cached();
static size_t round_up(size_t request_size, size_t pagesize);
static void *os_alloc(size_t nbytes);
static int os_release(void *ptr, size_t nbytes);

// Aligns pointer to the nearest 16 byte multiple
static void *align_ptr(void *p, size_t a) {
    uintptr_t ptr = (uintptr_t)p;
    ptr = (ptr + a - 1) & ~(uintptr_t)(a - 1);
    return (void *)ptr;
}

/*
 * Requests space from the OS, creating a new Chunk of memory
 * which can be used to allocate blocks to the request.
 *
 * Args:
 *  Size: The number of bytes to be allocated.
 * Returns:
 *  The first block in the new requested chunk.
 *  NULL if memory could not be allocated.
 */
static Block *request_space(size_t size) {
    size_t pagesize = get_pagesize_cached();

    size_t total = sizeof(Chunk) + HDRSIZE + size;
    total = round_up(total, CHUNK_SIZE);
    total = round_up(total, pagesize);

    // Request enough size for freelist and requested size
    void *ptr = os_alloc(total);

    if (!ptr) {
        return NULL;
    }

    // Store the Chunk data
    Chunk *chunk = (Chunk *)ptr;
    chunk->size = total;
    chunk->next = g_chunks;
    g_chunks = chunk;

    // Find position after chunk header, then reserve space for
    // block header. Then align the end of block head to start payload.
    char *after_header = (char *)(chunk + 1);
    char *aligned_block = (char *)align_ptr(after_header + HDRSIZE, ALIGN);

    // Calculate the total size that the block header and padding took.
    size_t padding = (size_t)(aligned_block - after_header);

    // Place first block in memory after the Chunk Header and aligned.
    Block *b = (Block *)(aligned_block - HDRSIZE);
    b->size = total - sizeof(Chunk) - padding + HDRSIZE;
    b->next = g_free_head;
    g_free_head = b;

    return b;
}

/*
 * Splits the block passed in *block*, after execution
 * *block* will have the necessary space for the request.
 * The new block created will be the return value.
 *
 * Args:
 *  block: The block to be split.
 *  total: The total size needed for the request, this
 *  includes the header and the payload size.
 * Returns:
 *  The new block created after the necessary split
 *  NULL if a split is not possible (not enough space)
 */
static Block *split(Block *block, size_t total) {
    // Enough size for the request, and also to split it
    if (block->size >= total + HDRSIZE + MIN_BLOCK) {
        Block *new = (Block *)((char *)block + total);
        new->size = block->size - total;
        new->next = block->next;

        block->size = total;
        block->next = NULL;

        return new;
    }

    // Not enough space to split the blocks
    return NULL;
}

/*
 * Finds the next free block using LIFO for the requested
 * size passed in through *payload_size*. Removing it
 * from the free list.
 *
 * Args:
 *  payload_size: the bytes that need to be allocated
 * Returns:
 *  The first block found with enough space for the request.
 *  NULL if no block is found
 */
static Block *find_free_block(size_t payload_size) {
    payload_size = round_up(payload_size, ALIGN);
    size_t need = payload_size + HDRSIZE;

    Block *current = g_free_head;
    Block *prev = NULL;

    while (current) {
        if (current->size >= need) {
            Block *new = split(current, need);
            if (new == NULL) {
                if (prev)
                    prev->next = current->next;
                else
                    g_free_head = current->next;
            } else {
                if (prev)
                    prev->next = new;
                else
                    g_free_head = new;
            }
            return current;
        }
        prev = current;
        current = current->next;
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
    return ((request_size + align - 1) / align) * align;
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

void *malloc(size_t size) {
    if (size <= 0) {
        return NULL;
    }

    Block *block;

    while ((block = find_free_block(size)) == NULL) {
        if (request_space(size) == NULL) {
            return NULL;
        }
    }

    return (char *)block + HDRSIZE;
}

void free(void *ptr) {
    if (!ptr) {
        return;
    }

    Block *block = (Block *)((char *)ptr - HDRSIZE);

    block->next = g_free_head;
    g_free_head = block;
}

void *realloc(void *ptr, size_t size) {
    if (ptr == NULL) {
        return malloc(size);
    }
    if (size == 0) {
        free(ptr);
        return NULL;
    }

    Block *oldb = (Block *)((char *)ptr - HDRSIZE);
    size_t old_total = oldb->size;
    size_t old_payload = (old_total >= HDRSIZE) ? (old_total - HDRSIZE) : 0;

    void *newp = malloc(size);
    if (!newp)
        return NULL;

    size_t to_copy = old_payload < size ? old_payload : size;
    if (to_copy)
        memcpy(newp, ptr, to_copy);

    free(ptr);
    return newp;
}

void *calloc(size_t n, size_t sz) {
    if (n && sz > (SIZE_MAX / n)) {
        return NULL;
    }
    size_t total = n * sz;

    void *p = malloc(total);
    if (!p)
        return NULL;
    memset(p, 0, total);
    return p;
}

// Helper functions to be defined when testing.
#ifdef MYMALLOC_TESTING
void *mymalloc_test_os_alloc(size_t n) { return os_alloc(n); }
int mymalloc_test_os_release(void *p, size_t n) { return os_release(p, n); }
size_t mymalloc_test_pagesize(void) { return get_pagesize_cached(); }
size_t mymalloc_test_roundup(size_t s, size_t pz) { return round_up(s, pz); }
Block *mymalloc_test_request_space(size_t size) { return request_space(size); }
Block *mymalloc_test_find_free(size_t payload_size) { return find_free_block(payload_size); }
Block *mymalloc_test_split(Block *block, size_t total) { return split(block, total); }
Block *mymalloc_test_free_head(void) { return g_free_head; }

// Release all chunks and reset global state.
void mymalloc_test_reset(void) {
    Chunk *c = g_chunks;
    while (c) {
        Chunk *next = c->next;
        os_release(c, c->size);
        c = next;
    }
    g_chunks = NULL;
    g_free_head = NULL;
    g_pagesize = 0;
}

size_t mymalloc_test_block_size_of_payload(void *payload) {
    Block *b = (Block *)((char *)payload - HDRSIZE);
    return b->size;
}
size_t mymalloc_test_free_head_size(void) {
    return g_free_head ? g_free_head->size : 0;
}

size_t mymalloc_test_hdrsize(void) { return HDRSIZE; }
size_t mymalloc_test_min_block(void) { return MIN_BLOCK; }

#endif
