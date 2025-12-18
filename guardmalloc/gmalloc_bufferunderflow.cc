#include "guardmalloc.h"
#include <cstdio>
#include <cstring>

// This program intentionally crashes due to a buffer underflow
// (writing before the start of allocated memory into the lower guard page)
int main() {
    printf("Testing lower guard page detection (buffer underflow)...\n");
    fflush(stdout);
    
    // Allocate a small buffer
    int *ptr = (int*)GMALLOC(sizeof(int) * 5);
    if (!ptr) {
        fprintf(stderr, "Failed to allocate memory\n");
        return 1;
    }
    
    // Write to the allocated memory
    for (int i = 0; i < 5; i++) {
        ptr[i] = i * 10;
    }
    printf("Allocated and initialized array of 5 integers\n");
    
    // Intentionally write before the start of the allocation into the guard page
    // This should cause a crash
    printf("Attempting to write before allocated memory (buffer underflow)...\n");
    fflush(stdout);
    ptr[-10] = 999;  // Writing before the allocated region
    
    return 0;
}
