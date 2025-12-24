#include "tagheap.h"
#include <stdlib.h>

/**
 * This file contains 
 */

typedef struct memblock_s {
    struct memblock_s *prev;
    struct memblock_s *next;
    uint32_t tag : 12;
    uint32_t size : 20;
} memblock_t;

static uint8_t heap[MH_HEAPSIZE];

#define FIRSTBLOCK ((memblock_t *)&heap[0])
#define LASTBLOCK ((memblock_t *)&heap[MH_HEAPSIZE-sizeof(memblock_t)])

// Init the heap with start and end markers indicating the free space
void MH_init() {
    // Initialize the start and end markers
    FIRSTBLOCK->next = LASTBLOCK;
    FIRSTBLOCK->prev = NULL;
    FIRSTBLOCK->tag = MH_FREE_TAG;
    FIRSTBLOCK->size = MH_HEAPSIZE-2*sizeof(memblock_t);
    LASTBLOCK->next = NULL;
    LASTBLOCK->prev = FIRSTBLOCK;
    LASTBLOCK->tag = MH_FREE_TAG;
    LASTBLOCK->size = 0;
}

// Allocate *size* bytes and give them the given tag. Start search in head
uint8_t *MH_alloc_head(int size, short tag){
    // Find first block that can hold the requested size starting from bottom


}

// Allocate *size* bytes and give them the given tag. Start search in tail
uint8_t *MH_alloc_tail(int size, short tag){

}

// Free all blocks with tags between tag_low and tag_high both included.
void MH_freetags(short tag_low, short tag_high){

}

// Free a single block pointed to by ptr
void MH_free(uint8_t *ptr){

}

// Defrag head as described in tagheap.h. Both tag_low and tag_high is included
// in the range to be defragmented.
void MH_defrag(defrag_cb_t callback, short tag_low, short tag_high){

}