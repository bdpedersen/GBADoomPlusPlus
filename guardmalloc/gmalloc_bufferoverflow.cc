#include "guardmalloc.h"
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <unistd.h>

// This program intentionally crashes due to a buffer overflow
// (writing past the end of allocated memory into the upper guard page)
int main() {
    printf("Testing upper guard page detection (buffer overflow)...\n");
    fflush(stdout);
    
    // Get page size to know how far we need to overflow
    size_t pagesize = sysconf(_SC_PAGESIZE);
    
    // Allocate 100 bytes
    // This will be rounded up to the next page boundary for the data region
    // Data region = header (24 bytes) + user data (100 bytes) = 124 bytes
    // Rounded up to nearest page: ceil(124 / pagesize) * pagesize
    char *ptr = (char*)GMALLOC(100);
    if (!ptr) {
        fprintf(stderr, "Failed to allocate memory\n");
        return 1;
    }
    
    // Write to the allocated memory
    ptr[0] = 'A';
    printf("Allocated and wrote to memory\n");
    
    // Intentionally write past the allocation into the guard page
    printf("Attempting to write past allocated memory (buffer overflow)...\n");
    fflush(stdout);
    
    // Write far past the boundary
    // The data region is one page in size (header + 100 bytes rounded up)
    // We need to write past pagesize bytes of the user data pointer to hit the guard
    // Since header is 24 bytes before user data, we need to write to indices >= (pagesize - 24)
    for (int i = pagesize; i < (int)(pagesize + 100); i++) {
        ptr[i] = 'X';
    }
    
    printf("Should not reach here!\n");
    fflush(stdout);
    return 0;
}
