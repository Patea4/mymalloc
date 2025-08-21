
**Growth Policy**: When additional memory is needed the allocator requests more memory from the OS of size:

`ceil(max(CHUNK_SIZE, request_size_total) / PAGE_SIZE) * PAGE_SIZE`

This is done to request the minimum amount of pages possible from the OS.

CHUNK_SIZE is defined to be 64KiB, which is the minimum amount of memory to request from the OS.
