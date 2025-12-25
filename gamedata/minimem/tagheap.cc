#include "tagheap.h"
#include <stdlib.h>

/**
 * This file contains 
 */

typedef struct memblock_s {
    struct memblock_s *prev;
    struct memblock_s *next;
    uint16_t tag;
    uint16_t size4;
} memblock_t;

#define SZ_MEMBLOCK (sizeof(memblock_t)/4)

static uint32_t heap[TH_HEAPSIZE4];

#define FIRSTBLOCK ((memblock_t *)&heap[0])
#define LASTBLOCK ((memblock_t *)&heap[TH_HEAPSIZE4-SZ_MEMBLOCK])

// Init the heap with start and end markers indicating the free space
void TH_init() {
    // Initialize the start and end markers
    FIRSTBLOCK->next = LASTBLOCK;
    FIRSTBLOCK->prev = NULL;
    FIRSTBLOCK->tag = TH_FREE_TAG;
    FIRSTBLOCK->size4 = TH_HEAPSIZE4-2*SZ_MEMBLOCK;
    LASTBLOCK->next = NULL;
    LASTBLOCK->prev = FIRSTBLOCK;
    LASTBLOCK->tag = TH_FREE_TAG;
    LASTBLOCK->size4 = 0;
}

static inline int bytesize2size(int bytesize) {
    int roundup = (bytesize | (bytesize >> 1)) & 1;
    return (bytesize >> 2) + roundup;
}

static inline bool is_tail(uint16_t tag) {
    return tag & 0x8000;
}

// Allocate *size* bytes and give them the given tag. Start search in head
static uint32_t *alloc_head(int bytesize, uint16_t tag){
    // Find first block txhat can hold the requested size starting from bottom
    int size4 = bytesize2size(bytesize);
    int searchsize = size4 + SZ_MEMBLOCK; // Make sure to have space for a block header
    auto block = FIRSTBLOCK;
    while (block && 
        ((block->tag==TH_FREE_TAG && block->size4 < searchsize) 
        || !is_tail(block->tag) )) {
            block = block->next;
    }
    // Now block will either point to a suitable free area or be null
    if (block && block->tag == TH_FREE_TAG) {
        // Insert a new header above the block and make room for <<size>> bytes
        // Mark the new header as free memory 
        int newsize = block->size4-SZ_MEMBLOCK-size4;
        uint32_t *newptr = (uint32_t *)block + SZ_MEMBLOCK + size4;
        memblock_t *newblock = (memblock_t *)newptr;
        block->next->prev = newblock;
        newblock->next = block->next;
        newblock->prev = block;
        block->next=newblock;
        newblock->size4 = newsize;
        // The data is south of the new block
        newblock->tag = TH_FREE_TAG;
        block->tag=tag;
        block->size4 = size4;
        return (uint32_t *)(block+1);
    }
    return NULL;
}

// Allocate *size* bytes and give them the given tag. Start search in tail
static uint32_t *alloc_tail(int bytesize, short tag){
    // Find first block txhat can hold the requested size starting from bottom
    int size4 = bytesize2size(bytesize);
    int searchsize = size4 + SZ_MEMBLOCK; // Make sure to have space for a block header
     auto block = LASTBLOCK;
    while (block && 
        ((block->tag==TH_FREE_TAG && block->size4 < searchsize) 
        || is_tail(block->tag))) {
            block = block->prev;
    }
    // Now block will either point to a suitable free area or be null
    if (block && block->tag == TH_FREE_TAG) {
        // Insert a new header above the block and make room for <<size>> bytes
        // Mark the new header as free memory 
        int newsize4 = block->size4-SZ_MEMBLOCK-size4;
        // Put the new block right below the next block
        uint32_t *newptr = (uint32_t *)block->next - SZ_MEMBLOCK - size4;
        memblock_t *newblock = (memblock_t *)newptr;
        block->next->prev = newblock;
        newblock->next = block->next;
        newblock->prev = block;
        block->next=newblock;
        // The data area is north of newblock
        block->size4 = newsize4;
        block->tag = TH_FREE_TAG;
        newblock->tag=tag;
        newblock->size4 = size4;
        return (uint32_t *)(newblock+1);
    }
    return NULL;
}

uint32_t *TH_alloc(int bytesize, uint16_t tag) {
    return is_tail(tag) ? alloc_tail(bytesize,tag) : alloc_head(bytesize,tag);
}

static int freeblock(memblock_t *block){
    memblock_t *next = block->next;
    memblock_t *prev = block->prev;
    int freetype = 0;
    // Allow merge with next block only if it is not the end marker
    freetype |= (next && next->tag == TH_FREE_TAG && next->size4 > 0) ? 1 : 0;
    freetype |= (prev && prev->tag == TH_FREE_TAG) ? 2 : 0;

    int freed = block->size4;
    // Compute how many memblock allocations we save below
    for (int i=freetype; i ; i>>=1) {
        freed += SZ_MEMBLOCK;
    }
    switch (freetype) {
        case 1: // Merge with next, removing next block - unless it is next-to-last block
            block->size4 += next->size4+SZ_MEMBLOCK;
            block->next = next->next;
            next->next->prev = block;
        case 0: // block is a new free island
            block->tag = TH_FREE_TAG;
            break;
        case 2: // Merge with previous, removing this block
            prev->size4 += block->size4 + SZ_MEMBLOCK;
            prev->next = block->next;
            next->prev = prev;
            break;
        case 3: // Merge block and next with previous
            prev->size4 += block->size4 + next->size4 + 2*SZ_MEMBLOCK;
            prev->next = next->next;
            next->next->prev = prev;
        default:
            break;
    }
    return freed*4;
}

// Free all blocks with tags between tag_low and tag_high both included.
void TH_freetags(uint16_t tag_low, uint16_t tag_high){
    auto block = FIRSTBLOCK;
    while (block) {
        if (tag_low <= block->tag && block->tag <= tag_high) {
            freeblock(block);
        }
    }
}

// Free a single block pointed to by ptr
void TH_free(uint32_t *ptr){
    memblock_t *block = (memblock_t *)ptr;
    freeblock(block-1);
}

// Defrag head as described in tagheap.h. Both tag_low and tag_high is included
// in the range to be defragmented. Never set tag_high to TH_FREE_TAG
void TH_defrag(defrag_cb_t move_if_allowed){
    auto block = FIRSTBLOCK;
    // TODO: Implement defragmentation
    while (block) {
        if (block->tag == TH_FREE_TAG && block->size4 > 0){
            if (block->next->tag == TH_FREE_TAG) {
                // Reconcile the two blocks unless we reached the end
                if (block->next->size4 > 0) {
                    block->size4 += block->next->size4 + SZ_MEMBLOCK;
                    block->next = block->next->next;
                    block->next->prev = block;
                    // Retry the move
                    continue;
                }
            } else {
                // We may have a block we can move ... 
                memblock_t *next = block->next;
                // We are into the tail - no need to continue defrag
                if (is_tail(next->tag)) break;
                // Else check if we can move it
                uint32_t *newaddr = (uint32_t *)(block+1);
                if (move_if_allowed(next->tag,newaddr)){
                    // Move allowed
                    memblock_t oldblock = *next;
                    uint32_t *dst = newaddr;
                    uint32_t *src = (uint32_t *)(next+1);
                    for (int n=0; n<next->size4; n++) {
                        *dst++=*src++;
                    }
                    // We are effectively flipping the free space in block 
                    // with the data area in block->next, so the sizes and 
                    // tags of the two swaps. We then insert a new block after the 
                    // new data area that holds the new free space. Consolidation
                    // with eventual free space will be done next round. 
                    memblock_t *newblock = (memblock_t *)dst;
                    newblock->size4 = block->size4; 
                    newblock->tag = TH_FREE_TAG;
                    newblock->next = oldblock.next;
                    newblock->next->prev = newblock;
                    block->tag = oldblock.tag;
                    block->size4 = oldblock.size4;
                    block->next = newblock;
                    newblock->prev = block;
                }
            }
        } else {
            // We are into the tail - no need to terminate defrag
            if (is_tail(block->tag)) break;
        }
        block = block->next;
    }
}

int TH_countfreehead() {
    memblock_t *block = FIRSTBLOCK;
    int free = 0;
    // Step through and find all free blocks until we meet a tail block or reach the end
    while (block) {
        if (block->tag == TH_FREE_TAG) {
            free += block->size4;
        } else {
            if (is_tail(block->tag)) {
                break;
            }
        }
    }
    return free*4;
}