#include "../newcache/newcache.h"
#include "../include/annontations.h"
#include "../include/r_defs.h"

extern unsigned char gfx_stbar[];
extern line_t junk;



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
    // Violent hack !
    if (lumpnum == STBAR_LUMP_NUM){
        return (const uint8_t *)gfx_stbar; 
    }
    if (lumpnum == JUNK_LUMP_NUM){
        return (const uint8_t *)&junk;
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

const char* NC_GetNameForNum(int lump, char buffer[8])
{
    const char* name = W_GetNameForNum(lump);
    strncpy(buffer,name,8);
    return buffer;
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

void NC_FlushCache(void)
{
    // No-op for this simple cache
}