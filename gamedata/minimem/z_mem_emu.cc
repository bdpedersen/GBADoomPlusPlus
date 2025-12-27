/**
 * Memory emulation for z_zone and z_bmalloc functions using tagheap
 */

 #include "tagheap.h"
 #include <stdlib.h>
 #include <memory.h>
 #include <z_bmalloc.h>
 #include <annotations.h>
 #include <stdlib.h>
 

bool NC_FreeSomeMemoryForTail(int bytes);

 /// Z_BMalloc replacement
 void * Z_BMalloc(block_memory_alloc_s *zone) {
    unsigned tag = zone->tag | 0x80000000; // Ensure MSB is set for tail allocation
    auto ptr = TH_alloc(zone->size, tag);
    if (!ptr) {
        // Try to free some memory - ask for double the size to be safe
        if (!NC_FreeSomeMemoryForTail(2*zone->size) || !(ptr = TH_alloc(zone->size, tag))) {
            // Out of memory - this is fatal
            #if TH_CANARY_ENABLED == 1
            printf("FATAL: Z_BMalloc: Out of memory trying to allocate %zu bytes with tag %u\n", zone->size, tag);
            #endif
            exit(-1);
        }
    }
    return ptr;
 }

 void Z_BFree(struct block_memory_alloc_s *pzone UNUSED, void* p){
    TH_free((uint8_t *)p);
}