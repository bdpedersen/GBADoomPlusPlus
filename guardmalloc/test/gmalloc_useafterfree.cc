#include "../guardmalloc.h"
#include <cstdio>
#include <cstring>

// This program intentionally crashes due to use-after-free
int main() {
    printf("Testing use-after-free detection...\n");
    fflush(stdout);
    
    // Allocate some memory
    int *ptr = (int*)GMALLOC(sizeof(int) * 10);
    if (!ptr) {
        fprintf(stderr, "Failed to allocate memory\n");
        return 1;
    }
    
    // Write to the allocated memory
    ptr[0] = 42;
    printf("Allocated and wrote to memory: ptr[0] = %d\n", ptr[0]);
    
    // Free the memory
    GFREE(ptr);
    printf("Freed the memory\n");
    
    // Intentionally try to read from freed memory - this should crash
    printf("Attempting to read from freed memory: ptr[0] = %d\n", ptr[0]);
    fflush(stdout);
    
    return 0;
}
