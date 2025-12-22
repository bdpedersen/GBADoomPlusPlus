#include "../newcache/newcache.h"
#include "../guardmalloc/guardmalloc.h"

#include <stdio.h>
#include <string.h>
#include <map>

/**
 * This file contains a simple cache that uses guardmalloc to allocate memory and 
 * agressively disposes memory to trigger guardmalloc's leak detection in case some 
 * pointers are stolen from the cache functions. 
 */ 

const void * W_CacheLumpNum(int lumpnum);
int W_LumpLength(int lumpnum);
int W_GetNumForName (const char* name);
int W_CheckNumForName(const char *name);
const char *W_GetNameForNum(int lumpnum);
void W_Init(void);
void ExtractFileBase(const char* path, char* dest);

int cachedlump = -1;
const uint8_t *cacheddata = nullptr;

std::map<int,const uint8_t *> pinned_allocations;
std::map<int,int> pincount;

// Simple wrappers mapping to W_ functions in the newcache namespace
const uint8_t * NC_CacheLumpNum(int lumpnum)
{
    if (lumpnum == STBAR_LUMP_NUM){
        return (const uint8_t *)gfx_stbar; // Violent hack !
    }

    if (cachedlump != lumpnum){
        // Free previous cache
        if (cacheddata){
            // Don't free pinned allocations
            if (pinned_allocations.count(cachedlump) == 0){
                GFREE((void *)cacheddata);
            }
            cacheddata = nullptr;
            cachedlump = -1;
        }
        // Allocate new cache
        int len = W_LumpLength(lumpnum);
        uint8_t *data = (uint8_t *)GMALLOC(len);
        if (!data){
            printf("NC_CacheLumpNum: Failed to allocate %d bytes for lump %d\n", len, lumpnum);
            exit(-1);
        }
        const uint8_t *lumpdata = (const uint8_t *)W_CacheLumpNum(lumpnum);
        memcpy(data, lumpdata, len);
        cacheddata = data;
        cachedlump = lumpnum;
        //printf(".");
        //fflush(stdout);
    }
    return cacheddata;
}  

int NC_LumpLength(int lumpnum)
{
    return W_LumpLength(lumpnum);
}

int NC_GetNumForName (const char* name)
{
    return W_GetNumForName(name);
}

int NC_CheckNumForName(const char *name)
{
    return W_CheckNumForName(name);
}

const char* NC_GetNameForNum(int lump)
{
    return W_GetNameForNum(lump);
}

void NC_Init(void)
{
    W_Init();
}

void NC_ExtractFileBase(const char* path, char* dest)
{
    ExtractFileBase(path, dest);
}

const uint8_t * NC_Pin(int lumpnum)
{
    if (pincount.count(lumpnum)){
        pincount[lumpnum]+=1;
        printf("\nRepinning lump %d, pincount now: %d\n",lumpnum,pincount[lumpnum]);
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
}

void NC_Unpin(int lumpnum)
{
    if (pincount.count(lumpnum) == 0){
        printf("Error: Lump %d is not pinned\n", lumpnum);
        exit(-1);
    }

    if (--pincount[lumpnum]) return; // Nested pin - not time to unpin yet

    // If the pinned allocation is not the cached one, free it
    if (cachedlump != lumpnum){
        GFREE((void *)pinned_allocations[lumpnum]);
    }

    pinned_allocations.erase(lumpnum);
    pincount.erase(lumpnum);
}   

