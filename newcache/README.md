# NewCache 
NewCache is an optimization that allows GBADoom to run on systems with very little RAM and memory mapped ROM/FLASH, like microcontrollers. The WAD file in doom is a little less than 4Mb and in GBADoom it is put in the game ROM. NewCache makes it possible to move this to external FLASH, also on devices that don't support memory mapped external flash, while keeping the need of RAM to a minimum. The original doom allows caching LUMPs loaded from the WAD file  in RAM by only requesting them from disk when they are needed, and releasing them when they are not needed. To save RAM this mechanism is partially disabled/broken in GBADoom as it is not necessary to cache LUMPs when they are readily available in memory mapped ROM.

NewCache reintroduces this mechanism in a more aggressive form where LUMPs can be evicted from RAM and reloaded triggered by OOM and pointer access respectively. This is done in the C++ introducing a smartpointer class to handle all LUMPs. Instead of loading the LUMP into memory and keeping it there until it is not needed anymore, each access to the pointer is checked and may cause a (re)load of the underlying data. This allows aggressive eviction of data from the RAM cache, assuming that it is cheap to reload it from external flash. 

## How it works
The smartpointer class doesn't hold a pointer per se, but the ID of the LUMP that it represents, which is then turned into an actual pointer on each access. If the object is already in memory, the pointer lookup is fairly cheap and does this:

- Convert lump ID to a cache index
- Convert cache index to a pointer
- Update an LRU list

but if not it will be loaded from flash which may entail the following:

- Free up memory if not enough is available by evicting object(s) from the cache
- Defragment heap if not enough contiguous memory is available
- Allocate memory and load data from flash
- Update the free/LRU list
- Return the pointer

Having the pointer re-materialize on each access allows lumps to move around in physical memory, and thereby to address heap fragmentation. This means that it is possible to move much closer to the theoretical memory lower limit by being much more aggressive on reloading objects - assuming external FLASH access is cheap.

## Implementation details

### LumpInfo
LumpInfo is an array with enough entries to hold information about all available lumps in the WAD. For GBADoom with Doom1.wad this means 1198 entries. Each entry looks like this

``` c++
typedef struct {
    uint32_t wad_offset;
    uint32_t size;
    uint8_t cacheID;
} lumpinfo_t;
```

wad_offset and size are discovered during initialization by parsing the file system in the WAD file. Note that lookup by name will require a procedure that will actually traverse the WAD file directly. cacheID is the field that allows the pointer lookup. If the ID is 0, the object is not in memory and will need to be loaded from cache.

The cache structure is a 256-entry array of CacheItems. it looks like this
```c++ 
struct cacheitem {
    void *data;
    union {
        uint8_t prev;
        uint8_t next_unused;
    }
    uint8_t next;
} cacheindex[256];
```

The array constitutes an indexed doubly linked list, so that efficient LRU tracking can be done. It also keeps track of unused cache indices by a singly linked list. 

Index 0 is special as it never points to any data. Instead it constitutes the top of both lists. cacheindex[0].next is the index of the most recently used entry, and cacheindex[0].next_unused is the index of the first unused entry. Entry 255 is also special in that cacheindex[255].prev always points to the least recently used cached object. This structure allows very efficient tracking of LRU, and it also allows very efficient keeping track of unused items, while at the same time being compact. 

if the cache runs out of entries, cacheindex[0].next_unused will be 0, and object(s) needs to be evicted to free up space. Most likely heap runs out of space before cache, so this should be a rare case.

## Heap
The heap utilizes two features of the doom code, namely that

1) Static data is always allocated before per-level data (provided that per-level data is flushed between levels)
2) Per level data is rarely freed

So we will have one heap that supports both the lump cache and static data needed by the game during runtime. 

Static data is allocated in descending stack fashion from the top. Then a marker is put in and per-level data is allocated as requested, but never freed. This makes the allocation process cheap and removes the need for heap management for these objects

Cache entries are allocated from the bottom and is subject to the watermark set by the level data. The bigger the total heap, the less the need for eviction of during execution, but in theory the system will work with just space for the biggest LUMP after loading all static and level data (which should mean 200kbyte or less). A memory block has a header that looks like this:

```c++
struct memblock_s {
    struct memblock_s *prev;
    uint32_t inuse : 1;
    uint32_t bytesize : 19;
    uint32_t lump : 12;
} memblock_t
```

lump is needed during defrag operations, as the cache entry will need its pointer updated when the block is moved. size is used to track where the next block starts. inuse is used for tracking a free block between two occupied blocks. Size is then the free area. When a block is freed due to a cache eviction, the system checks if the preceeding block or next block are free as well. If so, they are coalesced into one larger free block. 

cacheindex[0].data always points to the first memory block, and cacheindex[255].data always points to the last memory block, which will - by definition - not be in use. Upon initialization, these will be the same, and size will represent the total size of the heap. It will be adjusted if the block is freed or if level data eats into the free area of the cache. When memory is allocated for level data after lumps has been loaded, this can cause lumps to get evicted and the watermark to drop. But 

### Heap allocation
Allocation of lump data follows this procedure: 
1) If enough free space is available in the last block, use that, otherwise find the first free memory block with enough space to hold the requested data. When found mark it as in-use. If 264 bytes or more are available after allocation of data, insert a new memory block right after the allocated one, marked as unused, with the remaining size (not counting the memory block size)
2) If no such block exists, defrag the heap by traversing all blocks and copying down when an unused block is encountered. Repeat at (1)

Allocation of level data follows this procedure:
1) If enough free space is available in the last block, just reduce the size of that. 
2) If not enough space was available, evict least recently used data until enough is evicted to make room. Don't bother to defrag yet
3) Defrag heap as described above, and go to (1)

### Heap free
When changing level, all level data is evicted by simply rolling back to the watermark set by the static allocation

Freeing a memory block means setting its inuse flag to 0, and then check if either previous or next block are free. If they are, coaelsce the two blocks into one free one. Check if the new free block should move the last block marker. The cache index should then be moved to unused. This is done by using the lump index to look up the cache index, and then set its next_free to cacheindex[0].next_free and cacheindex[0].nextfree to index. cacheID for the lump is then set to 0.