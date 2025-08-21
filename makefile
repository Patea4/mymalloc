
CFLAGS ?= -O2 -Wall -Wextra

# Test library, exports mymalloc_test_* functions
testlib: libmymalloc_test.so
libmymalloc_test.so: mymalloc.c mymalloc.h tests/mymalloc_test.h
	gcc -shared -fPIC $(CFLAGS) -DMYMALLOC_TESTING -o $@ $<


test: os_alloc_tests.out
os_alloc_tests.out: tests/os_alloc_tests.c libmymalloc_test.so tests/mymalloc_test.h
	gcc $(CFLAGS) -o $@ $< -L. -lmymalloc_test -Wl,-rpath,.


clean:
	rm -f libmymalloc.so libmymalloc_test.so os_alloc_tests.out
