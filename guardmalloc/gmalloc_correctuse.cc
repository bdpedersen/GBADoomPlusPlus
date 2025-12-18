#include "guardmalloc.h"
#include <cstdio>
#include <cstring>
#include <unistd.h>

// This program uses gmalloc correctly
int main() {
    printf("Testing correct gmalloc usage...\n");
    fflush(stdout);
    
    size_t pagesize = sysconf(_SC_PAGESIZE);
    size_t alloc_size = pagesize + 500;
    
    // Allocate memory for pagesize + 500 bytes
    char *ptr = (char*)GMALLOC(alloc_size);
    if (!ptr) {
        fprintf(stderr, "Failed to allocate memory\n");
        return 1;
    }
    
    printf("Allocated %zu bytes\n", alloc_size);
    
    // Write to the beginning of the allocation
    printf("Writing to beginning of allocation...\n");
    for (size_t i = 0; i < 100; i++) {
        ptr[i] = 'A' + (i % 26);
    }
    
    // Verify writes at the beginning
    printf("Verifying beginning: ");
    for (size_t i = 0; i < 10; i++) {
        printf("%c", ptr[i]);
    }
    printf("...\n");
    
    // Write to the end of the allocation
    printf("Writing to end of allocation...\n");
    for (size_t i = alloc_size - 100; i < alloc_size; i++) {
        ptr[i] = 'Z' - ((i - (alloc_size - 100)) % 26);
    }
    
    // Verify writes at the end
    printf("Verifying end: ");
    for (size_t i = alloc_size - 10; i < alloc_size; i++) {
        printf("%c", ptr[i]);
    }
    printf("...\n");
    
    // Allocate additional memory to verify multiple allocations
    int *arr = (int*)GMALLOC(sizeof(int) * 10);
    if (!arr) {
        fprintf(stderr, "Failed to allocate array\n");
        return 1;
    }
    
    // Initialize and use the array
    for (int i = 0; i < 10; i++) {
        arr[i] = i * 10;
    }
    printf("Allocated and initialized array of 10 integers\n");
    
    // Free both allocations
    GFREE(arr);
    printf("Freed integer array memory\n");
    
    GFREE(ptr);
    printf("Freed large allocation memory\n");
    
    // Check for memory leaks
    gcheckleaks();
    printf("All memory properly freed!\n");
    fflush(stdout);
    
    return 0;
}
