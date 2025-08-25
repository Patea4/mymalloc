
CFLAGS ?= -O2 -Wall -Wextra
CFLAGS += -g -O0 -Wformat -Wformat-security -fno-omit-frame-pointer

# Test library, exports mymalloc_test_* functions
testlib: libmymalloc_test.so
libmymalloc_test.so: mymalloc.c mymalloc.h tests/mymalloc_test.h
	gcc -shared -fPIC $(CFLAGS) -DMYMALLOC_TESTING -o $@ $<


test: os_alloc_tests.out alignment_tests.out freelist_tests.out malloc_align_tests.out

os_alloc_tests.out: tests/os_alloc_tests.c libmymalloc_test.so tests/mymalloc_test.h
	gcc $(CFLAGS) -o $@ $< -L. -lmymalloc_test -Wl,-rpath,.

alignment_tests.out: tests/alignment_tests.c libmymalloc_test.so tests/mymalloc_test.h
	gcc $(CFLAGS) -o $@ $< -L. -lmymalloc_test -Wl,-rpath,.

freelist_tests.out: tests/freelist_tests.c libmymalloc_test.so tests/mymalloc_test.h
	gcc $(CFLAGS) -o $@ $< -L. -lmymalloc_test -Wl,-rpath,.

malloc_align_tests.out: tests/malloc_align_tests.c libmymalloc_test.so tests/mymalloc_test.h
	gcc $(CFLAGS) -o $@ $< -L. -lmymalloc_test -Wl,-rpath,.


clean:
	rm -f *.so *.out
