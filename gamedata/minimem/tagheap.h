#ifndef __memheap_h
#define __memheap_h

#include <stdint.h>


/**
 * This API is for a tagged memory allocator. It supports allocation from both 
 * head and tail of the heap area, and it supports defragmentation of the head
 * area. Furthermore tag ranges can be freed in one go.
 * 
 * Tags with MSB set are allocated in the tail, while tags with MSB cleared are
 * allocated in the head. Only head tags can be defragmented
 * 
 * It is meant to be used as central heap for DOOM, allocating static objects 
 * and level objects at the tail end, while the head end is used for caching
 * Cached<> objects. Idea is to have them interfere as little as possible and
 * efficiently allocate and free memory as the game goes on.
 * 
 * Defragmentation is done by traversing memory blocks from head end and copying
 * down into free areas which bubbles up free memory to the end. For each memory
 * block a callback is called with the tag and the (proposed) new address. If 
 * the callback returns true, the block is moved to the new address, while if it
 * returns false, the move is vetoed. This is to protect pinned memory where a 
 * raw pointer is currently in flight. Defragmentation will only try to move blocks
 * in the given tag range.
 * 
 * 
 */


// Heap size in 32-bit words
#ifndef TH_HEAPSIZE
#define TH_CACHEHEAPSIZE 180000 // bytes - portion of heap used for caching
#define TH_OBJECTHEAPSIZE 250000 // bytes - portion of heap used for static and level objects
#define TH_HEAPSIZE (TH_CACHEHEAPSIZE+TH_OBJECTHEAPSIZE)
#endif

#ifndef TH_CANARY_ENABLED
#define TH_CANARY_ENABLED 1
#endif

#if TH_CANARY_ENABLED == 1
#include <stdio.h>
#endif
typedef struct th_memblock_s {
    #if TH_CANARY_ENABLED == 1
    uint32_t canary[4]; // Canary to detect memory corruption
    #endif
    struct th_memblock_s *prev;
    struct th_memblock_s *next;
    uint32_t tag;   // Tag of this block
    uint32_t size;  // Size in bytes
} th_memblock_t;

extern uint8_t th_heap[TH_HEAPSIZE];
#define FIRSTBLOCK ((th_memblock_t *)&th_heap[0+(tag >> 31)*TH_CACHEHEAPSIZE])

#define TH_FREE_TAG 0xffffffff


typedef bool (*defrag_cb_t)(short tag, uint8_t *proposed_newptr);

uint8_t *TH_alloc(int bytesize, uint32_t tag);
uint8_t *TH_realloc(uint8_t *ptr, int newsize);
void TH_freetags(uint32_t tag_low, uint32_t tag_high);
// Return the amount of bytes freed by this free (including headers)
int TH_free(uint8_t *ptr);
void TH_defrag(defrag_cb_t callback);
void TH_init();
// Count how many bytes are free for allocation into head.
int TH_countfreehead();

bool TH_checkhealth_verbose();


#endif // __memheap_h