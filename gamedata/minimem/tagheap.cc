#include "tagheap.h"
#include <stdlib.h>

/**
 * This file contains 
 */



#define SZ_MEMBLOCK sizeof(th_memblock_t)

uint8_t heap[TH_HEAPSIZE];

#define FIRSTBLOCK ((th_memblock_t *)&heap[0])
#define LASTBLOCK ((th_memblock_t *)&heap[TH_HEAPSIZE-SZ_MEMBLOCK])

// Init the heap with start and end markers indicating the free space
void TH_init() {
    // Initialize the start and end markers
    FIRSTBLOCK->next = LASTBLOCK;
    FIRSTBLOCK->prev = NULL;
    FIRSTBLOCK->tag = TH_FREE_TAG;
    FIRSTBLOCK->size = TH_HEAPSIZE-2*SZ_MEMBLOCK;
    LASTBLOCK->next = NULL;
    LASTBLOCK->prev = FIRSTBLOCK;
    LASTBLOCK->tag = TH_FREE_TAG;
    LASTBLOCK->size = 0;
}

constexpr int log2ceil(int x) {
    int r = 0;
    int v = 1;
    while (v < x) {
        v <<= 1;
        r++;
    }
    return r;
}

static inline bool is_tail_or_free(uint32_t tag) {
    return (tag & 0x80000000);
}

static inline unsigned nearest4up(int x) {
    return (x + 3) & ~3;
}

// Allocate *size* bytes and give them the given tag. Start search in head
static uint8_t *alloc_head(int bytesize, uint32_t tag){
    // Find first block txhat can hold the requested size starting from bottom
    unsigned size = nearest4up(bytesize); // Align to 4 bytes
    unsigned searchsize = size + SZ_MEMBLOCK; // Make sure to have space for a block header
    auto block = FIRSTBLOCK;
    while (block && 
        ((block->tag==TH_FREE_TAG && block->size < searchsize) 
        || (!is_tail_or_free(block->tag)) )) {
            block = block->next;
    }
    // Now block will either point to a suitable free area or be null
    if (block && block->tag == TH_FREE_TAG) {
        // Insert a new header above the block and make room for <<size>> bytes
        // Mark the new header as free memory 
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
        return (uint8_t *)(block+1);
    }
    return NULL;
}

// Allocate *size* bytes and give them the given tag. Start search in tail
static uint8_t *alloc_tail(int bytesize, uint32_t tag){
    // Find first block txhat can hold the requested size starting from bottom
    unsigned size = nearest4up(bytesize); // Align to 4 bytes
    unsigned searchsize = size + SZ_MEMBLOCK; // Make sure to have space for a block header
     auto block = LASTBLOCK;
    while (block && 
        ((block->tag==TH_FREE_TAG && block->size < searchsize) 
        || (block->tag!=TH_FREE_TAG && is_tail_or_free(block->tag)))) {
            block = block->prev;
    }
    // Now block will either point to a suitable free area or be null
    if (block && block->tag == TH_FREE_TAG) {
        // Insert a new header above the block and make room for <<size>> bytes
        // Mark the new header as free memory 
        int newsize = block->size-SZ_MEMBLOCK-size;
        // Put the new block right below the next block
        uint8_t *newptr = (uint8_t *)block->next - SZ_MEMBLOCK - size;
        th_memblock_t *newblock = (th_memblock_t *)newptr;
        block->next->prev = newblock;
        newblock->next = block->next;
        newblock->prev = block;
        block->next=newblock;
        // The data area is north of newblock
        block->size = newsize; 
        block->tag = TH_FREE_TAG;
        newblock->tag=tag;
        newblock->size = bytesize; // may be overallocated
        return (uint8_t *)(newblock+1);
    }
    return NULL;
}

uint8_t *TH_alloc(int bytesize, uint32_t tag) {
    return is_tail_or_free(tag) ? alloc_tail(bytesize,tag) : alloc_head(bytesize,tag);
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
            block->tag = TH_FREE_TAG;
            break;
        case 1: // Merge with next, removing next block - unless it is next-to-last block
            block->size += next->size+SZ_MEMBLOCK;
            block->next = next->next;
            next->next->prev = block;
            block->tag = TH_FREE_TAG;
            break;
        case 2: // Merge with previous, removing this block
            prev->size += block->size + SZ_MEMBLOCK;
            prev->next = block->next;
            next->prev = prev;
            break;
        case 3: // Merge block and next with previous
            prev->size += block->size + next->size + 2*SZ_MEMBLOCK;
            prev->next = next->next;
            next->next->prev = prev;
            break;
        default:
            break;
    }
    return freed;
}

// Free all blocks with tags between tag_low and tag_high both included.
void TH_freetags(uint32_t tag_low, uint32_t tag_high){
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

// Defrag head as described in tagheap.h. Both tag_low and tag_high is included
// in the range to be defragmented. Never set tag_high to TH_FREE_TAG
void TH_defrag(defrag_cb_t move_if_allowed){
    auto block = FIRSTBLOCK;
    // TODO: Implement defragmentation
    while (block) {
        if (block->tag == TH_FREE_TAG && block->size > 0){
            if (block->next->tag == TH_FREE_TAG) {
                // Reconcile the two blocks unless we reached the end
                if (block->next->size > 0) {
                    block->size += block->next->size + SZ_MEMBLOCK;
                    block->next = block->next->next;
                    block->next->prev = block;
                    // Retry the move
                    continue;
                }
            } else {
                // We may have a block we can move ... 
                th_memblock_t *next = block->next;
                // We are into the tail - no need to continue defrag
                if (is_tail_or_free(next->tag)) break;
                // Else check if we can move it
                uint8_t *newaddr = (uint8_t *)(block+1);
                if (move_if_allowed(next->tag,newaddr)){
                    // Move allowed
                    th_memblock_t oldblock = *next;
                    uint8_t *dst = newaddr;
                    uint8_t *src = (uint8_t *)(next+1);
                    for (unsigned n=0; n<next->size; n++) {
                        *dst++=*src++;
                    }
                    // We are effectively flipping the free space in block 
                    // with the data area in block->next, so the sizes and 
                    // tags of the two swaps. We then insert a new block after the 
                    // new data area that holds the new free space. Consolidation
                    // with eventual free space will be done next round. 
                    th_memblock_t *newblock = (th_memblock_t *)dst;
                    newblock->size = block->size; 
                    newblock->tag = TH_FREE_TAG;
                    newblock->next = oldblock.next;
                    newblock->next->prev = newblock;
                    block->tag = oldblock.tag;
                    block->size = oldblock.size;
                    block->next = newblock;
                    newblock->prev = block;
                }
            }
        } else {
            // We are into the tail - no need to terminate defrag
            if (is_tail_or_free(block->tag)) break;
        }
        block = block->next;
    }
}

int TH_countfreehead() {
    th_memblock_t *block = FIRSTBLOCK;
    int free = 0;
    // Step through and find all free blocks until we meet a tail block or reach the end
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