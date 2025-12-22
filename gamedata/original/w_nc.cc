#include "../newcache/newcache.h"

const void * W_CacheLumpNum(int lumpnum);
int W_LumpLength(int lumpnum);
int W_GetNumForName (const char* name);
int W_CheckNumForName(const char *name);
const char *W_GetNameForNum(int lumpnum);
void W_Init(void);
void ExtractFileBase(const char* path, char* dest);

// Simple wrappers mapping to W_ functions in the newcache namespace
const void * NC_CacheLumpNum(int lumpnum)
{
    return W_CacheLumpNum(lumpnum);
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
