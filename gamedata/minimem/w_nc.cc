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

static uint8_t *cache[MAXLUMPS];

static wadinfo_t header;

static int allocated = 0;

static void InitCache() {
    for (int i=0;i<MAXLUMPS;i++)
        cache[i]=nullptr;
    cache[STBAR_LUMP_NUM]=gfx_stbar;
    cache[JUNK_LUMP_NUM]=(uint8_t *)&junk;
}

static filelump_t LumpForNum(int lumpnum){
    int offset = header.infotableofs+lumpnum*sizeof(filelump_t);
    filelump_t data;
    WR_Read((uint8_t*)&data,offset,sizeof(filelump_t));
    return data;
}

// Simple wrappers mapping to W_ functions in the newcache namespace
const uint8_t * NC_CacheLumpNum(int lumpnum)
{
    if (cache[lumpnum]==nullptr){
        // Allocate new cache entry and load it from file
        auto lump = LumpForNum(lumpnum);
        uint8_t *data = (uint8_t *)TH_alloc(lump.size,lumpnum);
        allocated += lump.size;
        if (!data){
            printf("NC_CacheLumpNum: Failed to allocate %d bytes for lump %d\n", lump.size, lumpnum);
            exit(-1);
        }else {
            printf("Allocated %d bytes for lump %d (#%d bytes)\n", lump.size, lumpnum, allocated);
        }
        // Read the header
        WR_Read(data,lump.filepos,lump.size);
        cache[lumpnum]=data;
        return cache[lumpnum];
    } else {
        return cache[lumpnum];
    }
}  

int NC_LumpLength(int lumpnum)
{
    // TODO: Grab length from cache if the element is already cached.
    auto data = LumpForNum(lumpnum);

    return data.size;
}

int NC_GetNumForName (const char* name)
{
    int i = NC_CheckNumForName(name);
    if (i==-1) {
        printf("lump %s not found\n",name);
    }
    return i;
}

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

const char* NC_GetNameForNum(int lump, char buffer[8])
{
    // This is never cached so ...
    uint64_t *nbuf = (uint64_t *)buffer;
    auto thelump = LumpForNum(lump);
    *nbuf = thelump.nname;
    return buffer;
}

void NC_Init(void)
{
    WR_Init();
    TH_init();
    // Permanently pin lumps that are allocated in normal RAM
    InitCache();
    // Read the header
    WR_Read((uint8_t *)&header,0,sizeof(header));
}

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

const uint8_t * NC_Pin(int lumpnum)
{
    /*
    if (lumpnum==-1) return nullptr;

    if (pincount.count(lumpnum)){
        pincount[lumpnum]+=1;
        return pinned_allocations[lumpnum];
    }

    // Pin the current cached data if it matches
    if (cachedlump == lumpnum && cacheddata){
        pinned_allocations[lumpnum] = cacheddata;
        pincount[lumpnum]=1;
        return cacheddata;
    }

    // Else cache it anew
    auto data = NC_CacheLumpNum(lumpnum);
    pinned_allocations[lumpnum] = data;
    pincount[lumpnum]=1;
    return data;
    */
    auto data = NC_CacheLumpNum(lumpnum);
    return data;
}

void NC_Unpin(int lumpnum UNUSED)
{
    /*
    if (lumpnum == -1) return;
    if (pincount.count(lumpnum) == 0){
        printf("Error: Lump %d is not pinned\n", lumpnum);
        exit(-1);
    }

    if (--pincount[lumpnum]) return; // Nested pin - not time to unpin yet

    // If the pinned allocation is not the cached one, free it
    if (cachedlump != lumpnum && lumpnum > -2){
        GFREE((void *)pinned_allocations[lumpnum]);
    }

    pinned_allocations.erase(lumpnum);
    pincount.erase(lumpnum);
    */
}   

void NC_FlushCache(void)
{
    printf("Flushing cache with %d bytes in it\n",allocated);
    allocated = 0;
    TH_freetags(0, MAXLUMPS);
    InitCache();
}


