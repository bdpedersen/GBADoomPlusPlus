#ifndef __memheap_h
#define __memheap_h

#include <stdint.h>

/**
 * This API is for a tagged memory allocator. It supports allocation from both 
 * head and tail of the heap area, and it supports defragmentation of the head
 * area. Furthermore tag ranges can be freed in one go
 * 
 * It is meant to be used as central heap for DOOM, allocating static objects 
 * and level objects at the tail end, while the head end is used for caching
 * Cached<> objects. 
 * 
 * Defragmentation is done by traversing memory blocks from head end and copying
 * down into free areas which bubbles up free memory to the end. For each memory
 * block a callback is called with the tag and the (proposed) new address. If 
 * the callback returns true, the block is moved to the new address, while if it
 * returns false, the move is vetoed. This is to protect pinned memory where a 
 * raw pointer is currently in flight. Defragmentation will only try to move blocks
 * in the given tag range.
 */

#ifndef MH_HEAPSIZE
#define MH_HEAPSIZE 256000
#endif

#define MH_FREE_TAG 0xfff

typedef bool (*defrag_cb_t)(short tag, uint8_t *proposed_newptr);

uint8_t *MH_alloc_head(int size, short tag);
uint8_t *MH_alloc_tail(int size, short tag);
void MH_freetags(short tag_low, short tag_high);
void MH_free(uint8_t *ptr);
void MH_defrag(defrag_cb_t callback, short tag_low, short tag_high);
void MH_init();

#endif // __memheap_h