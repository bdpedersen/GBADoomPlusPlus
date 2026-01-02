#include "../gamedata/minimem/wadreader.h"
#include "../newcache/newcache.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

FILE *wad;

void WR_Init(){
    wad = fopen("gbadoom1.wad","rb");
    if (!wad) {
        printf("Couldn't open WAD file\n");
        assert(false);exit(-1);
    }
}

void WR_Read(uint8_t *dst, int offset, int len) {
    fseek(wad,offset,SEEK_SET);
    fread(dst,1,len,wad);
}
