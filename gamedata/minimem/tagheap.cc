#include "tagheap.h"
//#include <stdlib.h>
#if TH_CANARY_ENABLED == 1
#include <stdio.h>
#include <assert.h>
#else
#define assert(x)
#endif

#include <stddef.h>

/**
 * This file contains a simple tagged memory allocator that supports allocation 
 */

static bool initialized = false;


#define SZ_MEMBLOCK sizeof(th_memblock_t)

uint8_t th_heap[TH_HEAPSIZE];

#define LASTBLOCK ((th_memblock_t *)&th_heap[TH_CACHEHEAPSIZE-SZ_MEMBLOCK+(tag >> 31)*TH_OBJECTHEAPSIZE])

// Init the heap with start and end markers indicating the free space
void TH_init() {
    if (initialized) return;
    initialized = true;
    // Initialize the start and end markers
    unsigned tag = 0;
    do {
        FIRSTBLOCK->next = LASTBLOCK;
        FIRSTBLOCK->prev = NULL;
        FIRSTBLOCK->tag = TH_FREE_TAG;
        FIRSTBLOCK->size = ((tag) ? TH_OBJECTHEAPSIZE : TH_CACHEHEAPSIZE)-2*SZ_MEMBLOCK;
        LASTBLOCK->next = NULL;
        LASTBLOCK->prev = FIRSTBLOCK;
        LASTBLOCK->tag = TH_FREE_TAG;
        LASTBLOCK->size = 0;
        #if TH_CANARY_ENABLED == 1
        // Fill in canary values
        for (int i=0; i<4; i++) {
            FIRSTBLOCK->canary[i] = 0xDEADBEEF;  
            LASTBLOCK->canary[i] = 0xDEADBEEF;  
        }
        #endif
        tag ^= 0x80000000; // Toggle between cache and objects
    } while (tag);
}



static inline bool is_tail_or_free(uint32_t tag) {
    return (tag & 0x80000000);
}

static inline unsigned nearest4up(unsigned x) {
    return (x + 3) & ~3;
}

// Allocate *size* bytes and give them the given tag. Start search in bottom
static uint8_t *alloc_first_fit(int bytesize, uint32_t tag){
    // Find first block txhat can hold the requested size starting from bottom
    unsigned size = nearest4up(bytesize); // Align to 4 bytes
    unsigned searchsize = size; // Make sure to have space for a block header and meaningful data
    auto block = FIRSTBLOCK;
    while (block && ((block->tag==TH_FREE_TAG && block->size < searchsize) 
         || block->tag != TH_FREE_TAG /*|| (!is_tail_or_free(block->tag))*/ )) { // Disable tail check here as this function now allocates in both heaps
            block = block->next;
    }
    // Now block will either point to a suitable free area or be null
    if (block && block->tag == TH_FREE_TAG) {
        // Insert a new header above the block and make room for <<size>> bytes
        // Mark the new header as free memory 
        if (block->size < size + SZ_MEMBLOCK + 16) {
            // Not enough space to split the block meaningfully - just allocate whole block and set size correctly
            block->size = bytesize;
            block->tag = tag;
            return (uint8_t *)(block+1);
        }
        int newsize = block->size-SZ_MEMBLOCK-size;
        uint8_t *newptr = (uint8_t *)block + SZ_MEMBLOCK + size;
        auto newblock = (th_memblock_t *)newptr;
        block->next->prev = newblock;
        newblock->next = block->next;
        newblock->prev = block;
        block->next=newblock;
        newblock->size = newsize;
        // The data is south of the new block
        newblock->tag = TH_FREE_TAG;
        block->tag=tag;
        block->size = bytesize; // may be overallocated
        #if TH_CANARY_ENABLED == 1
        // Fill in canary values in the new block
        for (int i=0; i<4; i++) {
            newblock->canary[i] = 0xDEADBEEF;
        }
        #endif
        return (uint8_t *)(block+1);
    }
    return NULL;
}

// Allocate *size* bytes and give them the given tag. Start search in bottom
static uint8_t *alloc_best_fit(int bytesize, uint32_t tag){
    // Find first block txhat can hold the requested size starting from bottom
    unsigned size = nearest4up(bytesize); // Align to 4 bytes
    unsigned searchsize = size + SZ_MEMBLOCK + 16; // Make sure to have space for a block header and meaningful data
    auto block = FIRSTBLOCK;
    th_memblock_t *bestblock = NULL;
    while (block) {
        if (block->tag == TH_FREE_TAG && block->size >= searchsize) {
            if (!bestblock || block->size < bestblock->size) {
                bestblock = block;
            }
        }
        block = block->next;
    }
    block = bestblock;
    // Now block will either point to a suitable free area or be null
    if (block && block->tag == TH_FREE_TAG) {
        // Insert a new header above the block and make room for <<size>> bytes
        // Mark the new header as free memory 
        if (block->size < size + SZ_MEMBLOCK + 16) {
            // Not enough space to split the block meaningfully - just allocate whole block and set size correctly
            block->size = bytesize;
            block->tag = tag;
            return (uint8_t *)(block+1);
        }
        int newsize = block->size-SZ_MEMBLOCK-size;
        uint8_t *newptr = (uint8_t *)block + SZ_MEMBLOCK + size;
        auto newblock = (th_memblock_t *)newptr;
        block->next->prev = newblock;
        newblock->next = block->next;
        newblock->prev = block;
        block->next=newblock;
        newblock->size = newsize;
        // The data is south of the new block
        newblock->tag = TH_FREE_TAG;
        block->tag=tag;
        block->size = bytesize; // may be overallocated
        #if TH_CANARY_ENABLED == 1
        // Fill in canary values in the new block
        for (int i=0; i<4; i++) {
            newblock->canary[i] = 0xDEADBEEF;
        }
        #endif
        return (uint8_t *)(block+1);
    }
    return NULL;
}

uint8_t *TH_alloc(int bytesize, uint32_t tag) {
    #if TH_CANARY_ENABLED == 1
    printf("TH_alloc: Requesting %d bytes with tag %u (%s)\n", bytesize, tag, is_tail_or_free(tag) ? "objects" : "cache");
    #endif
    auto ptr = (tag) ? alloc_best_fit(bytesize,tag) : alloc_first_fit(bytesize,tag); ;
    return ptr;
}

uint8_t *TH_realloc(uint8_t *ptr, int newsize) {
    int newsize_aligned = nearest4up(newsize);
    if (!ptr) return NULL;
    if (newsize <= 0) {
        TH_free(ptr);
        return NULL;
    }
    th_memblock_t *block = (th_memblock_t *)(ptr) - 1;
    if (block->size >= (uint32_t)newsize) {
        // Already big enough  
        return ptr;
    } else {
        // Find out if we can expand into next block
        th_memblock_t *next = block->next;
        if (next->tag == TH_FREE_TAG && (block->size + SZ_MEMBLOCK + next->size) >= (uint32_t)newsize_aligned) {
            // Can expand here
            int extra_needed = newsize_aligned - block->size;
            // CHeck if we leave a reasonable block size behind if we split
            // We are only moving the next block header up so no need to account for 
            // that blocksize
            if (next->size > (uint32_t)extra_needed + 16) {
                // Split the next block
                uint8_t *newblockptr = (uint8_t *)(next) + extra_needed;
                th_memblock_t *newblock = (th_memblock_t *)newblockptr;
                newblock->tag = TH_FREE_TAG;
                newblock->size = next->size - extra_needed;
                newblock->next = next->next;
                newblock->prev = block;
                next->next->prev = newblock;
                block->next = newblock;
                block->size = newsize;
            } else {
                // Just take the whole next block
                block->size = newsize; // May be overallocated - but that's ok
                block->next = next->next;
                next->next->prev = block;   
            }
            return ptr;
        } else {
            // Need to allocate a new block and move data
            uint8_t *newptr = TH_alloc(newsize, block->tag);
            if (newptr) {
                // Copy old data
                uint8_t *src = ptr;
                uint8_t *dst = newptr;
                for (unsigned n=0; n<block->size; n++) {
                    *dst++ = *src++;
                }
                TH_free(ptr);
                return newptr;
            }
        }
    }
    return NULL;
}

static int freeblock(th_memblock_t *block){
    th_memblock_t *next = block->next;
    th_memblock_t *prev = block->prev;
    int freetype = 0;
    // Allow merge with next block only if it is not the end marker
    freetype |= (next && next->tag == TH_FREE_TAG && next->size > 0) ? 1 : 0;
    freetype |= (prev && prev->tag == TH_FREE_TAG) ? 2 : 0;

    int freed = block->size;
    // Compute how many memblock allocations we save below
    for (int i=freetype; i ; i>>=1) {
        freed += SZ_MEMBLOCK;
    }
    switch (freetype) {
        case 0: // block is a new free island
        #if TH_CANARY_ENABLED == 1
            printf("Freeing block at %p of size %u creating an island\n", (void *)block, block->size);
        #endif
            block->tag = TH_FREE_TAG;
            break;
        case 1: // Merge with next, removing next block - unless it is next-to-last block
        #if TH_CANARY_ENABLED == 1
            printf("Freeing block at %p of size %u merging with next block at %p of size %u\n", (void *)block, block->size, (void *)next, next->size);
        #endif
            //block->size += next->size+SZ_MEMBLOCK;
            block->next = next->next;
            next->next->prev = block;
            block->tag = TH_FREE_TAG;
            break;
        case 2: // Merge with previous, removing this block
        #if TH_CANARY_ENABLED == 1
            printf("Freeing block at %p of size %u merging with previous block at %p of size %u\n", (void *)block, block->size, (void *)prev, prev->size);
        #endif
            //prev->size += block->size + SZ_MEMBLOCK;
            prev->next = block->next;
            next->prev = prev;
            block = prev;
            break;
        case 3: // Merge block and next with previous
        #if TH_CANARY_ENABLED == 1
            printf("Freeing block at %p of size %u merging with previous block at %p of size %u and next block at %p of size %u\n", (void *)block, block->size, (void *)prev, prev->size, (void *)next, next->size);
        #endif
            //prev->size += block->size + next->size + 2*SZ_MEMBLOCK;
            prev->next = next->next;
            next->next->prev = prev;
            block = prev;
            break;
        default:
            break;
    }
    // Adjust the size of the resulting free block
    block->size = (uintptr_t)(block->next) - (uintptr_t)(block+1);
    return freed;
}

// Free all blocks with tags between tag_low and tag_high both included.
void TH_freetags(uint32_t tag_low, uint32_t tag_high){
    assert (is_tail_or_free(tag_low) == is_tail_or_free(tag_high)); // Must be same type of tags
    unsigned tag = tag_low;
    auto block = FIRSTBLOCK;
    while (block) {
        if (tag_low <= block->tag && block->tag <= tag_high) {
            freeblock(block);
        }
        block = block->next;
    }
}

// Free a single block pointed to by ptr
int TH_free(uint8_t *ptr){
    th_memblock_t *block = (th_memblock_t *)ptr;
    return freeblock(block-1);
}

// Defrag cache as described in tagheap.h. Both tag_low and tag_high is included
// in the range to be defragmented. Never set tag_high to TH_FREE_TAG
void TH_defrag(defrag_cb_t move_if_allowed){
    unsigned tag = 0; // Only cache head blocks
    auto block = FIRSTBLOCK;
    // TODO: Implement defragmentation
    while (block) {
        if (block->tag == TH_FREE_TAG && block->size > 0){
            if (block->next->tag == TH_FREE_TAG) {
                // Reconcile the two blocks unless we reached the end
                if (block->next->size > 0) {
                    #if TH_CANARY_ENABLED == 1
                    printf("Defrag: Merging free block at %p of size %u with next free block at %p of size %u\n", (void *)block, block->size, (void *)(block->next), block->next->size);
                    #endif
                    block->size += block->next->size + SZ_MEMBLOCK;
                    block->next = block->next->next;
                    block->next->prev = block;
                    // Retry the move
                    continue;
                } else {
                    // Reached the end marker - stop defrag here
                    break;
                }
            } else {
                // We may have a block we can move ... 
                th_memblock_t *next = block->next;
                // We are into the objects - no need to continue defrag
                if (is_tail_or_free(next->tag)) break;
                // Else check if we can move it
                uint8_t *newaddr = (uint8_t *)(block+1);
                if (move_if_allowed(next->tag,newaddr)){
                    #if TH_CANARY_ENABLED == 1
                    printf ("Defrag: Moving block with tag %u from %p to %p\n", next->tag, (void *)(next), (void *)newaddr);
                    #endif
                    // Move allowed
                    th_memblock_t oldblock = *next;
                    // We can move using 32 bit load/stores as we know everything is 4 uint8_t aligned
                    uint32_t *dst = (uint32_t *)newaddr;
                    uint32_t *src = (uint32_t *)(next+1);
                    unsigned realsize = (next->size +3) & ~3; // Align to 4 bytes
                    for (unsigned n=0; n<(realsize>>2); n++) {
                        *dst++=*src++;
                    }
                    // We are effectively flipping the free space in block 
                    // with the data area in block->next, so the sizes and 
                    // tags of the two swaps. We then insert a new block after the 
                    // new data area that holds the new free space. Consolidation
                    // with eventual free space will be done next round. 
                    th_memblock_t *newblock = (th_memblock_t *)dst;
                    newblock->size = block->size; 
                    #if TH_CANARY_ENABLED == 1
                    uintptr_t avail = (uintptr_t)(oldblock.next) - (uintptr_t)(newblock+1);
                    assert(newblock->size <= avail);
                    #endif
                    newblock->tag = TH_FREE_TAG;
                    // Tying up the pointers in block and newblock + oldblock.next
                    newblock->next = oldblock.next;
                    newblock->next->prev = newblock;
                    #if TH_CANARY_ENABLED == 1
                    // Fill in canary values in the new block
                    for (int i=0; i<4; i++) {
                        newblock->canary[i] = 0xDEADBEEF;
                    }
                    #endif
                    // Update moved block header
                    block->tag = oldblock.tag;
                    block->size = oldblock.size;
                    block->next = newblock;
                    newblock->prev = block;
                }
            }
        } else {
            // We are into the objects - no need to terminate defrag
            if (is_tail_or_free(block->tag)) break;
        }
        block = block->next;
    }
}

int TH_countfreehead() {
    unsigned tag = 0; // only work on cache
    th_memblock_t *block = FIRSTBLOCK;
    int free = 0;
    // Step through and find all free blocks until we meet a objects block or reach the end
    while (block) {
        if (block->tag == TH_FREE_TAG) {
            free += block->size;
        } else {
            if (is_tail_or_free(block->tag)) {
                break;
            }
        }
        block = block->next;
    }
    return free;
}

bool TH_checkhealth_verbose() {
    #if TH_CANARY_ENABLED != 1
    return true;
    #else
    unsigned tag = 0;
    bool healthy = true;
    do {
        unsigned freemem = 0;
        unsigned contigfree = 0;
        printf("INFO: Checking heap health for %s allocations\n", (tag & 0x80000000) ? "object" : "cache");
        th_memblock_t *block = FIRSTBLOCK;
        th_memblock_t *prev = NULL;
        int cnt = 0;
        while (block) {
            #if TH_CANARY_ENABLED == 1
            // Check canary values
            for (int i=0; i<4; i++) {
                if (block->canary[i] != 0xDEADBEEF) {
                    printf("WARNING: Block #%d with tag %d has corrupted canary value at index %d\n", cnt, block->tag, i);
                    healthy = false;                
                }
            }
            #endif
            // Check backward link consistency
            if (block->prev != prev) {
                printf("WARNING: Block chain broken at block #%d with tag %d: backward link inconsistency - expected %p got %p. Previous block had tag %d\n", cnt, block->tag, (void *)prev, (void *)block->prev, prev ? prev->tag : -1);
                healthy = false;
            }
            if (block->next && ((uint8_t *)block->next < th_heap || 
                (uint8_t *)block->next >= th_heap + TH_HEAPSIZE)) {
                printf("WARNING: Block chain broken at block #%d  with tag %d: next pointer out of bounds\n", cnt, block->tag);
                healthy = false;
                break;
            }
            if (block->tag == TH_FREE_TAG) {
                freemem += block->size;
                contigfree = (contigfree > block->size) ? contigfree : block->size;
            }
            prev = block;
            block = block->next;
            cnt++;
        }
        if (healthy) {
            printf("INFO: %s Heap is healthy with %d blocks (%d bytes free - %d contiguous)\n", (tag) ? "Object" : "Cache",
                    cnt, freemem, contigfree);
        }
        tag ^= 0x80000000;
    } while (tag);
    return healthy;
    #endif
}
