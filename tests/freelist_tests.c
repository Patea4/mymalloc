#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "mymalloc_test.h"

const size_t MIN_BLOCK = 16;

static inline size_t hdr(void) { return mymalloc_test_hdrsize(); }
static inline size_t minb(void) { return mymalloc_test_min_block(); }

static inline size_t align_up(size_t x, size_t a) { return (x + a - 1) & ~(a - 1); }
static inline size_t align_down(size_t x, size_t a) { return x & ~(a - 1); }

static inline void assert_aligned(void *p) {
    assert(((uintptr_t)p % ALIGN) == 0 && "payload pointer not aligned");
}

// total bytes allocator takes for a payload
static inline size_t need_for_payload(size_t payload) {
    return align_up(payload, ALIGN) + hdr();
}

// payload you should request to *exact fit* a given block (or yield tiny remainder)
static inline size_t payload_for_exact_or_tiny(const Block *b) {
    // If (b->size - hdr) is multiple of ALIGN → exact fit.
    // Otherwise we rounded down → remainder < ALIGN (tiny), so allocator should take whole.
    return align_down(b->size - hdr(), ALIGN);
}

// payload that leaves a healthy remainder (>= hdr + MIN_BLOCK)
static inline size_t payload_for_healthy_split(const Block *b, size_t min_remainder) {
    size_t desired_need = b->size - (hdr() + min_remainder);
    size_t payload = align_down(desired_need - hdr(), ALIGN);
    // Recompute actual need after rounding; caller can assert remainder large enough.
    (void)desired_need;
    return payload;
}

static void test_exact_fit_or_tiny_remainder_takes_whole(void) {
    puts("[1] exact-fit or tiny remainder → take whole");

    mymalloc_test_reset();

    Block *B = mymalloc_test_request_space(32 * 1024);
    assert(B && "request_space failed");
    size_t S = B->size;

    size_t payload = payload_for_exact_or_tiny(B);
    Block *ret = mymalloc_test_find_free(payload);
    assert(ret && "find_free_block returned NULL");

    // Either exact fit or allocator takes the whole block on tiny remainder.
    assert(ret == B);
    assert(ret->size == S);
    assert(mymalloc_test_free_head() == NULL);

    assert_aligned((char *)ret + hdr());

    printf("ok: exact/tiny → took whole block (size=%zu)\n", S);
}

static void test_remove_middle_node(void) {
    puts("[2] remove middle node (exact or tiny remainder) ");

    mymalloc_test_reset();

    // Free list order is LIFO: head = B, then A
    Block *A = mymalloc_test_request_space(128 * 1024); // larger, second
    Block *B = mymalloc_test_request_space(16 * 1024);  // smaller, head
    assert(A && B);
    assert(mymalloc_test_free_head() == B);
    assert(B->size < A->size);

    size_t payloadA = payload_for_exact_or_tiny(A);
    // Also ensure we skip B: need must be > B->size
    assert(need_for_payload(payloadA) > B->size);

    Block *ret = mymalloc_test_find_free(payloadA);
    assert(ret == A);                       // removed the middle node
    assert(mymalloc_test_free_head() == B); // head stays B

    assert_aligned((char *)ret + hdr());
    puts("ok: removed A; head still B");
}

static void test_split_head(void) {
    puts("[3] split head");

    mymalloc_test_reset();

    Block *H = mymalloc_test_request_space(128 * 1024);
    assert(H);
    size_t S = H->size; // <— save ORIGINAL size

    // Leave remainder >= hdr + MIN_BLOCK (allocator can split)
    size_t payload = payload_for_healthy_split(H, minb());
    size_t need = need_for_payload(payload);
    size_t rem = S - need;
    assert(rem >= hdr() + minb());

    Block *ret = mymalloc_test_find_free(payload);
    assert(ret == H);
    assert(ret->size == need);

    Block *R = mymalloc_test_free_head(); // remainder becomes head
    assert(R == (Block *)((char *)H + need));
    assert(R->size == rem);
    assert(R->next == NULL);

    assert_aligned((char *)ret + hdr());
    assert_aligned((char *)R + hdr());

    assert(ret->size + R->size == S); // <— compare to original S

    puts("ok: split head -> sizes/links/alignment verified");
}

static void test_split_middle(void) {
    puts("[4] split middle");

    mymalloc_test_reset();

    Block *A = mymalloc_test_request_space(256 * 1024);
    Block *B = mymalloc_test_request_space(16 * 1024);
    assert(A && B && mymalloc_test_free_head() == B);

    size_t S_A = A->size; // <— save ORIGINAL size

    size_t payload = payload_for_healthy_split(A, minb());
    size_t need = need_for_payload(payload);
    size_t rem = S_A - need;

    assert(need > B->size);
    assert(rem >= hdr() + minb());

    Block *ret = mymalloc_test_find_free(payload);
    assert(ret == A);
    assert(ret->size == need);

    Block *head = mymalloc_test_free_head();
    assert(head == B);

    Block *R = B->next;
    assert(R == (Block *)((char *)A + need));
    assert(R->size == rem);

    assert_aligned((char *)ret + hdr());
    assert_aligned((char *)R + hdr());

    assert(ret->size + R->size == S_A); // <— compare to original S_A

    puts("ok: split middle -> sizes/links/alignment verified");
}

int main(void) {
    test_exact_fit_or_tiny_remainder_takes_whole();
    test_remove_middle_node();
    test_split_head();
    test_split_middle();

    puts("ALL FREELIST TESTS PASSED");
    return 0;
}
