#include "w_lumps.h"
#include <string.h>
#include <stdio.h>

#define CACHESIZE 16


static struct fl_cache_s {
   filelump_t lump;
   short index;
} _named_cache[CACHESIZE];
static struct fl_cache_s* named_cache = _named_cache - 1; // 1-based

#define LASTENTRY (CACHESIZE + 1)

static struct
{
    uint8_t next;
    union
    {
        uint8_t prev;
        uint8_t free;
    };
} lru[CACHESIZE + 2];

static void RemoveEntryFromLRU(uint8_t entry)
{
    auto prev = lru[entry].prev;
    auto next = lru[entry].next;

    lru[prev].next = lru[entry].next;
    lru[next].prev = lru[entry].prev;
}

static void InsertInFrontOfLRU(uint8_t entry)
{
    // 0 -> entry -> end
    // 0 <-       <- end
    lru[entry].prev = 0;
    lru[entry].next = lru[0].next;
    lru[lru[0].next].prev = entry;
    lru[0].next = entry;
}

static uint8_t GetFreeLRUEntry()
{
    uint8_t entry = lru[0].free;
    if (!entry)
    {
        // Remove least recently used
        entry = lru[LASTENTRY].prev;
        RemoveEntryFromLRU(entry);
    } else {
        // Take it out of the free list
        lru[0].free = lru[entry].free;
    }
    return entry;
}

int LC_CheckNumForName(const char *name)
{
    uint64_t nname = 0;
    strncpy((char *)&nname, name, 8);
    //check the named cache first
    uint8_t next_entry = lru[0].next;
    while (next_entry!= LASTENTRY)
    {
        if (named_cache[next_entry].lump.nname == nname)
        {
            //printf("Found in named cache: %s\n",name);
            // Mark this as most recently used
            RemoveEntryFromLRU(next_entry);
            InsertInFrontOfLRU(next_entry);
            return named_cache[next_entry].index;
        }
        next_entry = lru[next_entry].next;
    }
    //printf("Cache miss: %s\n",name);
    //not found in named cache - do full search
    uint32_t name_low = nname;
    uint32_t name_high = nname >> 32;
    for (int i = 0; i < WADLUMPS; i++)
    {
        if (lumpname_low[i] == name_low && lumpname_high[i] == name_high)
        {
            //insert into named cache
            uint8_t entry = GetFreeLRUEntry();
            named_cache[entry].lump.filepos = filepos[i];
            named_cache[entry].lump.size = lumpsize[i];
            named_cache[entry].lump.nname = nname;
            named_cache[entry].index = i;
            InsertInFrontOfLRU(entry);
            //printf("Caching lump %s at entry %d\n",name,entry);
            return i;
        }
    }
    // Now this is not found - we still need to register this
    // in the named cache as a missing entry:
    uint8_t entry = GetFreeLRUEntry();
    named_cache[entry].lump.filepos = 0;
    named_cache[entry].lump.size = 0;
    named_cache[entry].lump.nname = nname;
    named_cache[entry].index = -1;
    InsertInFrontOfLRU(entry);
    //printf("Caching missing lump %s at entry %d\n",name,entry);
    return -1;
}

const char *LC_GetNameForNum(int lump, char buffer[8])
{
    uint32_t name_low = lumpname_low[lump];
    uint32_t name_high = lumpname_high[lump];
    uint64_t nname = ((uint64_t)name_high << 32) | name_low;
    strncpy(buffer, (char *)&nname, 8);
    return buffer;
}

int LC_LumpLength(int lumpnum)
{
    return lumpsize[lumpnum];
}

filelump_t LC_LumpForNum(int lumpnum)
{
    filelump_t lump;
    lump.filepos = filepos[lumpnum];
    lump.size = lumpsize[lumpnum];
    uint32_t name_low = lumpname_low[lumpnum];
    uint32_t name_high = lumpname_high[lumpnum];
    uint64_t nname = ((uint64_t)name_high << 32) | name_low;
    lump.nname = nname;
    return lump;
}

void LC_Init(void)
{
    // Initialize LRU
    lru[0].next = LASTENTRY;
    lru[LASTENTRY].prev = 0;
    lru[LASTENTRY].next = 0;
    for (uint8_t i = 0; i < LASTENTRY-1; i++)
    {
        lru[i].free = i + 1;
    }
    lru[LASTENTRY-1].free = 0;
}
