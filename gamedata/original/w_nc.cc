#include "../newcache/newcache.h"
#include "../include/annontations.h"

#include <map>

std::map <int,const uint8_t *>tempptrs;

const void * W_CacheLumpNum(int lumpnum);
int W_LumpLength(int lumpnum);
int W_GetNumForName (const char* name);
int W_CheckNumForName(const char *name);
const char *W_GetNameForNum(int lumpnum);
void W_Init(void);
void ExtractFileBase(const char* path, char* dest);

// Simple wrappers mapping to W_ functions in the newcache namespace
const uint8_t * NC_CacheLumpNum(int lumpnum)
{
    if (tempptrs.count(lumpnum) > 0) return tempptrs[lumpnum];
    
    if (lumpnum == STBAR_LUMP_NUM){
        return (const uint8_t *)gfx_stbar; // Violent hack !
    }
    return (const uint8_t *)W_CacheLumpNum(lumpnum);
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

const uint8_t* NC_Pin(int lumpnum)
{
    return NC_CacheLumpNum(lumpnum); // We can assume it is constant in this implementaiton
}
void NC_Unpin(int lumpnum UNUSED)
{
    // No-op for this simple cache
}

int NC_Register(const uint8_t * ptr) {
    int lumpnum = -2;
    // Find next unused lumpnum
    for (; tempptrs.count(lumpnum)>0; lumpnum--);
    tempptrs[lumpnum]=ptr;
    return lumpnum;
}

void NC_Unregister(int lumpnum) {
    tempptrs.erase(lumpnum);
}