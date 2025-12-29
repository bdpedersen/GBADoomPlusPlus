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
        // Out of memory - this is fatal
        #if TH_CANARY_ENABLED == 1
        printf("FATAL: Z_BMalloc: Out of memory trying to allocate %zu bytes with tag %u\n", zone->size, tag);
        #endif
        exit(-1);
    }
    return ptr;
 }

 void Z_BFree(struct block_memory_alloc_s *pzone UNUSED, void* p){
    TH_free((uint8_t *)p);
}

/**
 * Z_Malloc replacement
 */
void * Z_Malloc(int size, int tag, void **user) {
    tag |= 0x80000000; // Ensure MSB is set for tail allocation
    // We need to also allocate space for the user pointer
    auto ptr = TH_alloc(size+sizeof(void**),tag);
    if (ptr) {
        auto userptr = (void **)ptr;
        ptr += sizeof(void**);
        if (user) {
            *userptr=user;
            *user = ptr;
        } else {
            *((void **)userptr)=(void **)2; // Mark as in use but unowned
        }
    } else {
        #if TH_CANARY_ENABLED == 1
        printf("FATAL: Z_Malloc: Out of memory trying to allocate %d bytes with tag %u\n", size, tag);
        #endif
        exit(-1);
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
    auto userptr = (void **)((uint8_t *)ptr - sizeof(void**));
    if (userptr > (void**)0x100) {
        void **user = (void **)(*userptr);
        if (user && user > (void **)0x100)
            *user = nullptr;
    }

    TH_free((uint8_t *)ptr-sizeof(void**));
}



/**
 * Z_Realloc replacement
 */
void * Z_Realloc(void *ptr, size_t n, int tag UNUSED, void **user UNUSED) {
    auto newptr = TH_realloc((uint8_t *)ptr,n);
    if (!newptr) {
        // Out of memory - this is fatal
        #if TH_CANARY_ENABLED == 1
        printf("FATAL: Z_Realloc: Out of memory trying to reallocate %zu bytes\n", n);
        #endif
        exit(-1);
    }
    return newptr;
}

/**
 * Z_Calloc replacement
 */
void * Z_Calloc(size_t count, size_t size, int tag, void **user) {
    auto ptr = Z_Malloc(size*count, tag, user);
    if (ptr) {
        // Zero out the memory
        auto realsize = ((size*count + 3) & ~3); // Align to 4 bytes
        auto ptr32 = (uint32_t *)ptr;
        for (unsigned n=0; n < (realsize >> 2); n++) {
            ptr32[n] = 0;
        }
    }
    #if TH_CANARY_ENABLED == 1
    if (TH_checkhealth_verbose()==false) {
        printf("FATAL: Heap corrupted after Z_Calloc of %zu bytes with tag %u\n",size*count,tag);
        exit(-1);
    }
    #endif
    return ptr;
}

/**
 * Z_FreeTags replacement
 */
void Z_FreeTags(int lowtag, int hightag) {
    unsigned tag = 0x80000000;
    th_memblock_t *block = FIRSTBLOCK;
    unsigned lt = lowtag | 0x80000000;
    unsigned ht = hightag | 0x80000000;
    while (block) {
        auto userptr = (uint8_t *)(block+1);
        auto ptr = userptr + sizeof(void**);
        if (lt <= block->tag && block->tag <= ht) {
            Z_Free(ptr);
        }
        block = block->next;
    }
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


