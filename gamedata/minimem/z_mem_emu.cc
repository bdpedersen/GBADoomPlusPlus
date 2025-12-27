/**
 * Memory emulation for z_zone and z_bmalloc functions using tagheap
 */

 #include "tagheap.h"
 #include <stdlib.h>
 #include <memory.h>
 #include <z_bmalloc.h>
 #include <z_zone.h>
 #include <annotations.h>
 #include <stdlib.h>
 

#undef Z_Malloc
#undef Z_Free
#undef Z_Realloc
#undef Z_Calloc
#undef Z_FreeTags

bool NC_FreeSomeMemoryForTail();

 /// Z_BMalloc replacement
 void * Z_BMalloc(block_memory_alloc_s *zone) {
    unsigned tag = zone->tag | 0x80000000; // Ensure MSB is set for tail allocation
    auto ptr = TH_alloc(zone->size, tag);
    if (!ptr) {
        // Try to free some memory - ask for double the size to be safe
        if (!NC_FreeSomeMemoryForTail() || !(ptr = TH_alloc(zone->size, tag))) {
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

/**
 * Z_Malloc replacement
 */
void * Z_Malloc(int size, int tag, void **user UNUSED) {
    tag |= 0x80000000; // Ensure MSB is set for tail allocation
    auto ptr = TH_alloc(size,tag);
    if (!ptr) {
        // Out of memory - this is fatal
        // Try to free some memory - ask for some extra to be safe
        #if TH_CANARY_ENABLED == 1
        printf("INFO: Z_Malloc: Evicing memory to allocate %d bytes with tag %u\n", size, tag);
        #endif
        if (!NC_FreeSomeMemoryForTail() || !(ptr = TH_alloc(size, tag))) {
            #if TH_CANARY_ENABLED == 1
            printf("FATAL: Z_Malloc: Out of memory trying to allocate %d bytes with tag %u\n", size, tag);
            #endif
            exit(-1);
        }
    }
    #if TH_CANARY_ENABLED == 1
    if (TH_checkhealth_verbose()==false) {
        printf("FATAL: Heap corrupted after Z_Malloc of %d bytes with tag %u\n",size,tag);
        exit(-1);
    }
    #endif
    return ptr;
}

/**
 * Z_Free replacement
 */
void Z_Free(void *ptr) {
    TH_free((uint8_t *)ptr);
}



/**
 * Z_Realloc replacement
 */
void * Z_Realloc(void *ptr, size_t n, int tag UNUSED, void **user UNUSED) {
    auto newptr = TH_realloc((uint8_t *)ptr,n);
    if (!newptr) {
        if (!NC_FreeSomeMemoryForTail() || !(ptr = TH_realloc((uint8_t *)ptr,n))) {
            // Out of memory - this is fatal
            #if TH_CANARY_ENABLED == 1
            printf("FATAL: Z_Realloc: Out of memory trying to reallocate %zu bytes\n", n);
            #endif
            exit(-1);
        }
    }
    return newptr;
}

/**
 * Z_Calloc replacement
 */
void * Z_Calloc(size_t count, size_t size, int tag, void **user UNUSED) {
    auto ptr = Z_Malloc(size*count, tag, user);
    // Zero out the memory
    auto realsize = ((size*count + 3) & ~3); // Align to 4 bytes
    auto ptr32 = (uint32_t *)ptr;
    for (unsigned n=0; n < (realsize >> 2); n++) {
        ptr32[n] = 0;
    }
    return ptr;
}

/**
 * Z_FreeTags replacement
 */
void Z_FreeTags(int lowtag, int hightag) {
    lowtag |= 0x80000000;
    hightag |= 0x80000000;
    TH_freetags(lowtag,hightag);
}

void Z_Init() {
    TH_init();
}

void Z_CheckHeap() {
    // NOP for now
}   

/**
 * Z_Strdup replacement lifted directly from original z_zone.cc
 */
char* Z_Strdup(const char* s)
{
    const unsigned int len = strlen(s);

    if(!len)
        return NULL;

    char* ptr = (char *)Z_Malloc(len+1, PU_STATIC, NULL);

    if(ptr)
        strcpy(ptr, s);

    return ptr;
}


