#include "../newcache/newcache.h"
#include "../guardmalloc/guardmalloc.h"
#include "../include/r_defs.h"
#include "wadreader.h"

#include <stdio.h>
#include <string.h>
#include "tagheap.h"

extern unsigned char gfx_stbar[];
extern line_t junk;

/**
 * This file contains a simple cache that uses TH_malloc to allocate memory and 
 * just keep it in memory until flushed.
 */ 

 /**
 * The LUT for indexing into the actual cache. 0 means that nothing is cached for this
 * 1-254 are valid cache indices. The last three has special meaning:
 * 252 : junk line_t
 * 253 : gfx_stbar
 * 254 : nullptr (used for the -1 index)
 */
static uint8_t _cache[MAXLUMPS+1];
static uint8_t *cache = _cache+1; // Allow -1 index



/**
 * The pointers that map cache indices to objects, either allocated on the heap using TH_alloc or by static mapping set up in InitCache()
 */
static uint8_t *pointers[256];

/**
 * The number of times an object has been pinned. Pinned objects can't be moved during a defrag, and they can't be evicted. Static mappings
 * are always pinned so that we never try to move or evict them. Pinning happens when a proxy object is converted to a pointer, either by
 * means of operator T*() on a Pnned<T> (produced by .pin()) or by means of a Sentinel implementing operator->()
 */
static uint8_t pincount[256];

/**
 * A doubly linked list implemented as an array. Start is index 0 and end is index 255, which is why cache indices can only be 1-254.
 * It serves two purposes:
 * 1) Track free entries by means of the .free singly linked list
 * 2) Track least recently used entries, by pushing an entry to the front each time it is pinned. 
 */
static struct {
    uint8_t next;
    union {
        uint8_t prev;
        uint8_t free;
    };
} lru[256];

/** 
 * The header for the WAD file. This is set up by WR_Init()
 */
static wadinfo_t header;

/**
 * Loosely tracking how much memory is allocated - for debug purposes
 */
static int allocated = 0;

/**
 * Cache initialization function. It:
 * - Sets all entries to 0 (unmapped)
 * - Prefills the special entries (-1 mapping to nullptr, STBAR_LUMP_NUM mapping to gfx_stbar and JUNK_LUMP_NUM mapping to junk)
 *   These objects are always pinned by setting pincount to 1 and thereby never have it 0 (pin and unpin is always symmetric)
 * - Initialize the free list to map to 1-251
 */
static void InitCache() {
    // Set all entries to 0 (unmapped)
    for (int i=-1;i<MAXLUMPS;i++)
        cache[i]=0;
    // Prefill special entries
    cache[-1]=254;
    pointers[254]=nullptr;
    pincount[254]=1; // Never deallocate
    cache[STBAR_LUMP_NUM]=253;
    pointers[253]=gfx_stbar;
    pincount[253]=1;
    cache[JUNK_LUMP_NUM]=252;
    pointers[252]=(uint8_t *)&junk;
    pincount[252]=1;
    // Set up the LRU - for pinning to work without special cases, the three constant ones must be part of it
    lru[0].next=252;
    lru[252].next=253;
    lru[253].next=254;
    lru[254].next=255;    
    lru[255].prev=254;
    lru[254].prev=253;
    lru[253].prev=252;
    lru[252].prev=0;
    for (int i=1; i<252; i++) {
        lru[i-1].free=i;
    }
    lru[251].free=0;
    allocated = 0;
}

/**
 * Helper function that can fetch the tag associated with ptr during TH_alloc()
 */
static uint32_t tag_for_ptr(const uint8_t *ptr) {
    auto block = (const th_memblock_t *)ptr;
    block -= 1;
    return block->tag;
}

/**
 * Helper function that can fetch the size in bytes associated with ptr during TH_alloc()
 */
static uint32_t size_for_ptr(const uint8_t *ptr){
    auto block = (const th_memblock_t *)ptr;
    block -= 1;
    return block->size;

}

/** 
 * Debug functin that prints the health of the heap in case of a fatal error.
 */
static void PrintHeapStatus() {
    uint8_t entry = 0;
    printf("\nHeap:\n");
    while (entry != 255) {
        uint8_t next_entry = lru[entry].next;
        const char* status = (lru[next_entry].prev==entry) ? "OK" : "Broken";
        const char* pinned = (pincount[next_entry]) ? "Pinned" : "Unpin";
        printf("%d %s %s(%d)\n",next_entry,pinned,status,lru[next_entry].prev);
        entry = next_entry;
    }
    printf("\nFreelist\n");
    entry=lru[0].free;
    while(entry) {
        printf("%d\n",entry);
        entry=lru[entry].free;
    }
}

/**
 * Helper function that removes entry from the LRU by linking the preceding and proceeding 
 * entries together
 */
static void RemoveEntryFromLRU(uint8_t entry) {
    auto prev = lru[entry].prev;
    auto next = lru[entry].next;

    lru[prev].next = lru[entry].next;
    lru[next].prev = lru[entry].prev;
}

/**
 * Helper function that inserts the entry at the front of the LRU
 */
static void InsertInFrontOfLRU(uint8_t entry) {
    // 0 -> entry -> end
    // 0 <-       <- end
    lru[entry].prev=0;
    lru[entry].next=lru[0].next;
    lru[lru[0].next].prev=entry;
    lru[0].next = entry;
}

/** Evict the least used non-pinned block. Return 0 if nothing can be evicted,
 * otherwise the number of bytes made available will be returned.
 */
static int EvictOne() {
    printf("INFO: Tryingn to evict one... ");
    uint8_t entry = lru[255].prev;
    while (entry && pincount[entry]) {
        entry = lru[entry].prev;
    }
    // If prev is 0, then we have nothing to evict - otherwise evict the least
    // recently used
    if (!entry) return 0;
    // Take it out of the LRU list and free it
    RemoveEntryFromLRU(entry);
    // Insert in free list
    lru[entry].free = lru[0].free;
    lru[0].free = entry;
    auto ptr = pointers[entry];
    auto tag = tag_for_ptr(ptr);
    cache[tag]=0; // Clear that entry from the cache
    allocated -= size_for_ptr(ptr);
    auto freed = TH_free(ptr);
    pointers[entry]=nullptr;
    printf(" lump %d from cache freeing %d bytes at entry %d\n",tag,freed,entry);
    return freed;
}

/**
 * Callback function for defragmentation
 */
static bool defrag_cb(short tag, uint8_t *proposed_newptr){
    auto entry = cache[tag];
    if (!entry) {
        printf("Warning: Defrag found an allocation that isn't mapped in the cache (lump=%d). This is a leak, and we will allow it to move\n",tag);
        return true;
    }
    if (pincount[entry]) return false; // Can't move pinned objects
    // Register the new pointer and give OK to move.
    pointers[entry]=proposed_newptr;
    return true;
}

/**
 * Allocate a new area in the cache and push it to the front of the LRU
 */
static uint8_t AllocateIntoCache(int bytes, int tag) {
    printf("\nINFO: Trying to allocate %d bytes for lump %d\n",bytes,tag);
    // Try to allocate bytes from the heap
    uint8_t *data = TH_alloc(bytes,tag);
    if (!data) {
        // We need to try harder - find out how much we have free
        int freemem = TH_countfreehead();
        // Then evict data until we have enough
        while (freemem < bytes){
            auto freed = EvictOne();
            freemem += freed;
            if (!freed) {
                printf("FATAL: Couldn't evict any useful amount..\n");
                PrintHeapStatus();
                exit(-1);
            }
        }
        // Try allocating again
        data = TH_alloc(bytes,tag);
        while (!data) {
            // We have enough free data but it is not contiguous - try defrag
            TH_defrag(defrag_cb);
            data = TH_alloc(bytes,tag);
            if (!data) {
                // Still not working - try to evict one and then try again
                if (!EvictOne()) {
                    // Now this is bad - we cant evict any more and we can't allocate. 
                    printf("FATAL: Can't allocate %d bytes for tag=%d\n",bytes,tag);
                    PrintHeapStatus();
                    exit(-1);
                }
            }
        }
    }
    // Get a free cache entry
    auto entry = lru[0].free;
    if (!entry) {
        // We need a free entry
        printf("INFO: Needs to evict a lump to free up a cache entry\n");
        if  (!EvictOne()){
            printf("FATAL: Cant evict an entry to free up - can't be true that all memory is pinned\n");
            PrintHeapStatus();
            exit(-1);
        } 
        entry = lru[0].free;
    }
    // Take it out of the free list
    lru[0].free = lru[entry].free;
    // Put it into pointers
    pointers[entry]=data;
    // Insert in front in the LRU.
    InsertInFrontOfLRU(entry);
    cache[tag]=entry;
    allocated += bytes;
    printf("INFO: Inserted %d bytes for lump %d into heap and cache structures at entry %d (@ 0x%lx)\n",bytes,tag,entry,(uintptr_t)data);
    return entry;
}

/**
 * Read a filelump_t from the wad file
 */
static filelump_t LumpForNum(int lumpnum){
    int offset = header.infotableofs+lumpnum*sizeof(filelump_t);
    filelump_t data;
    WR_Read((uint8_t*)&data,offset,sizeof(filelump_t));
    return data;
}

/**
 * Make sure that lumpnum is mapped and loaded in the cache and return the pointer to the lump data.
 */
const uint8_t * NC_CacheLumpNum(int lumpnum)
{
    if (cache[lumpnum]==0){
        // Allocate new cache entry and load it from file
        auto lump = LumpForNum(lumpnum);
        uint8_t entry = AllocateIntoCache(lump.size,lumpnum);
        auto ptr = pointers[entry];
        // Read the header
        WR_Read(ptr,lump.filepos,lump.size);    
    } 
    return pointers[cache[lumpnum]];
}  

/**
 * Return the size of the lump indexed by lumpnum, either by looking in the cache or by loading the
 * lump descriptor from the WAD file using LumpForNum and get the size from there.
 */
int NC_LumpLength(int lumpnum)
{
    // Grab length from cache if the element is already cached.
    uint8_t entry = cache[lumpnum];
    if (entry) {
        return size_for_ptr(pointers[entry]);
    }

    auto data = LumpForNum(lumpnum);
    return data.size;
}

/**
 * Lookup a lump by name and get its index, or -1 if not found
 */
int NC_GetNumForName (const char* name)
{
    int i = NC_CheckNumForName(name);
    if (i==-1) {
        printf("lump %s not found\n",name);
    }
    return i;
}

/**
 * Lookup a lump by name and get its index, or -1 if not found
 */
int NC_CheckNumForName(const char *name)
{
    uint64_t nname=0;
    strncpy((char *)&nname,name,8);
    int n = 0;
    while (n < header.numlumps) {
        int remaining_lumps = header.numlumps-n;
        int maxlumps = (remaining_lumps > 16) ? 16 : remaining_lumps;
        filelump_t lumps[16]; // 256 bytes
        // Read the lumps
        WR_Read((uint8_t *)lumps,header.infotableofs+n*sizeof(filelump_t),maxlumps*sizeof(filelump_t));
        for (int j=0; j < maxlumps;  j++) {
            if (nname == lumps[j].nname) {
                return n+j;
            }
        }
        n+=16;
    }
    
    return -1;
}

/**
 * Get the name of a given lump - return in buffer
 */
const char* NC_GetNameForNum(int lump, char buffer[8])
{
    // This is never cached so ...
    uint64_t *nbuf = (uint64_t *)buffer;
    auto thelump = LumpForNum(lump);
    *nbuf = thelump.nname;
    return buffer;
}

/**
 * Initialize newcache by initializing tagheap, wadreader and cache, and then reading the wad header
 */
void NC_Init(void)
{
    WR_Init();
    TH_init();
    // Permanently pin lumps that are allocated in normal RAM
    InitCache();
    // Read the header
    WR_Read((uint8_t *)&header,0,sizeof(header));
}

/**
 * Helper function normally implemented by w_wad - we implement here instead
 */
void NC_ExtractFileBase(const char* path, char* dest)
{
    // BDP: Lifted directly from w_wad
     const char *src = path + strlen(path) - 1;
    int length;

    // back up until a \ or the start
    while (src != path && src[-1] != ':' // killough 3/22/98: allow c:filename
           && *(src-1) != '\\'
           && *(src-1) != '/')
    {
        src--;
    }

    // copy up to eight characters
    memset(dest,0,8);
    length = 0;

    while ((*src) && (*src != '.') && (++length<9))
    {
        *dest++ = toupper(*src);
        src++;
    }
    /* cph - length check removed, just truncate at 8 chars.
   * If there are 8 or more chars, we'll copy 8, and no zero termination
   */
}

/**
 * Pin a lump in memory and return the (now stable) address. Also mark it as most recently used
 */
const uint8_t * NC_Pin(int lumpnum)
{
    auto data = NC_CacheLumpNum(lumpnum);
    auto entry = cache[lumpnum];
    //printf("Pinning lump %d from entry %d at address 0x%lx\n",tag_for_ptr(data),entry,(uintptr_t)data);
    pincount[entry]+=1;
    // Move entry up front in the LRU
    RemoveEntryFromLRU(entry);
    InsertInFrontOfLRU(entry);
    return data;
}

/**
 * Unpin the lump by decreasing its pincount
 */
void NC_Unpin(int lumpnum)
{
    //printf("Unpin lump %d\n",lumpnum);
    auto entry = cache[lumpnum];
    pincount[entry]-=1;    
}   

/**
 * Flush the cache by evicting all objects (could be done faster and more brute force, but this is easier to debug)
 */
void NC_FlushCache(void)
{
    printf("******************\n");
    printf("***    FLUSH   ***\n");
    printf("******************\n");
    printf("Flushing cache with %d bytes in it\n",allocated);
    while (EvictOne());
    printf("Flushed cache now has %d bytes in it\n",allocated);

    //TH_freetags(0, MAXLUMPS);
}


