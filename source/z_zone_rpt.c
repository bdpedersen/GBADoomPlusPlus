#include "../include/z_zone.h"
#include <stdio.h>

#ifdef RPT_MALLOC
#undef Z_Malloc
#undef Z_Free
#undef Z_Realloc
#undef Z_Calloc

static const char *redtext = "\033[31m";
static const char *yellowtext = "\033[33m";
static const char *greentext = "\033[32m";
static const char *bluetext = "\033[34m";
static const char *normaltext = "\033[0m";

static const char *tags[] = {
    "<NONE>",
    "PU_STATIC",
    "PU_LEVEL",
    "PU_LEVSPEC",
    "PU_CACHE"
};

static const char *tagcolors[] = {
    normaltext,
    redtext,
    yellowtext,
    greentext,
    bluetext
};

typedef struct memblock_s
{
    unsigned int size:24;	// including the header and possibly tiny fragments
    unsigned int tag:4;	// purgelevel
    void**		user;	// NULL if a free block
    struct memblock_s*	next;
    struct memblock_s*	prev;
} memblock_t;

static int tagcount[] = {
    0, 0, 0, 0, 0
};

static int maxtagcount[] = {
    0, 0, 0, 0, 0
};

static int tagcount_nofree[] = {
    0, 0, 0, 0, 0
};

static int Z_GetSize(void *ptr) {
    if(ptr == NULL)
        return 0;
    memblock_t *block = (memblock_t *)ptr;
    block--;

    return  block->size;
}   

static int Z_GetTag(void *ptr) {
    if(ptr == NULL)
        return 0;
    memblock_t *block = (memblock_t *)ptr;
    block--;
    return block->tag;
}

void* Z_MallocRpt(int size, int tag, void **ptr, const char* file, int line) {
    tagcount[tag] += size;
    tagcount_nofree[tag] += size;

    if(tagcount[tag] > maxtagcount[tag])
        maxtagcount[tag] = tagcount[tag];

    printf("%s:%d: %sAllocated %d bytes (%s=%d)%s\n", file, line, tagcolors[tag], size, tags[tag], tagcount[tag], normaltext);
    return Z_Malloc(size, tag, ptr);
}
void Z_FreeRpt(void *ptr, const char* file, int line) {
    int tag = Z_GetTag(ptr);
    int size = Z_GetSize(ptr);
    tagcount[tag] -= size;
    printf("%s:%d: %sFreed %d bytes (%s=%d)%s\n", file, line,  tagcolors[tag], size, tags[tag], tagcount[tag], normaltext);
    Z_Free(ptr);
}
void *Z_ReallocRpt(void *ptr, size_t n, int tag, void **user, const char* file, int line){
    int size_old = Z_GetSize(ptr);
    tagcount[tag] -= size_old;
    tagcount[tag] += n;

    if(tagcount[tag] > maxtagcount[tag])
        maxtagcount[tag] = tagcount[tag];
    if(tagcount[tag] > tagcount_nofree[tag])
        tagcount_nofree[tag] = tagcount[tag];

    printf("%s:%d: %sREALLOCATED from %d to %zu bytes (%s=%d)%s\n", file, line, tagcolors[tag], size_old, n, tags[tag], tagcount[tag], normaltext);
    return Z_Realloc(ptr, n, tag, user);
}
void *Z_CallocRpt(size_t count, size_t size, int tag, void **user, const char* file, int line) {
    tagcount[tag] += count * size;
    printf("%s:%d: %sAllocated %zu bytes (%s=%d) with Calloc%s\n", file, line, tagcolors[tag], count * size, tags[tag], tagcount[tag], normaltext);
    return Z_Calloc(count, size, tag, user);
}

void Z_ReportAll() {
    printf("Z_ReportAll: Current and maximum memory usage by tag:\n");
    for(int i = 0; i < 5; i++) {
        printf("%s%s: Current=%d bytes, Maximum=%d bytes\n%s", tagcolors[i],tags[i], tagcount[i], maxtagcount[i],normaltext);
    }
}

#endif
