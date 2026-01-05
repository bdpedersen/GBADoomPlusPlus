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
    // Emulate flash read speed - assume
    // Read 1 byte in 4 cycles (16 MHz, 4 bit per cycle)
    // Read 2 bytes from buffer and store in memory in 2 cycles
    // Overhead of 100 cycles for init and polling + 5 cycles/byte
    #ifdef __chess__
    uint32_t readtime = 5*len+100;
    uint32_t t0 = chess_cycle_count();
    #endif
    fseek(wad,offset,SEEK_SET);
    fread(dst,1,len,wad);
    #ifdef __chess__
    while (chess_cycle_count() < readtime+t0);
    #endif
}
