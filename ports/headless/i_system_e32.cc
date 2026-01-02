// PsionDoomDoc.cpp
//
// Copyright 17/02/2019 
//

#include "global_data.h"
#include "doomdef.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "i_system_e32.h"

#include "lprintf.h"

#include "annotations.h"

#ifndef __chess__
#include <time.h>
#endif

extern globals_t* _g;

//**************************************************************************************

unsigned int vid_width = 0;
unsigned int vid_height = 0;

unsigned int screen_width = 0;
unsigned int screen_height = 0;

unsigned int y_pitch = 0;

unsigned char* pb = NULL;
unsigned char* pl = NULL;


unsigned char* thearray = NULL;
int thesize;

unsigned short backbuffer[120 *160];
unsigned short frontbuffer[120 *160];


int I_GetTime(void)
{
    int thistimereply;

    #ifndef __chess__
    clock_t now = clock();

    // For microseconds we can do (37*time_us)>>20
    thistimereply = (int)((double)now / ((double)CLOCKS_PER_SEC / (double)TICRATE));
    #else
    #define MCYCLES_PER_SEC 32
    uint64_t cycles_us = chess_cycle_count()/MCYCLES_PER_SEC; // Or other us timer
    thistimereply = (cycles_us*37)>>20; // Approx. 35/1000000
    #endif

    if (thistimereply < _g->lasttimereply)
    {
        _g->basetime -= 0xffff;
    }

    _g->lasttimereply = thistimereply;


    /* Fix for time problem */
    if (!_g->basetime)
    {
        _g->basetime = thistimereply;
        thistimereply = 0;
    }
    else
    {
        thistimereply -= _g->basetime;
    }

    return thistimereply;
}


//**************************************************************************************

void I_InitScreen_e32()
{
	//Gives 480px on a 5(mx) and 320px on a Revo.
    vid_width = 120;

    vid_height = screen_height = 160;
}

//**************************************************************************************

void I_BlitScreenBmp_e32()
{

}

//**************************************************************************************

void I_StartWServEvents_e32()
{        

}

//**************************************************************************************

void I_PollWServEvents_e32()
{

}

//**************************************************************************************

void I_ClearWindow_e32()
{

}

unsigned short* I_GetBackBuffer()
{
    return &backbuffer[0];
}

unsigned short* I_GetFrontBuffer()
{
    return &frontbuffer[0];
}

//**************************************************************************************

void I_CreateWindow_e32()
{

}

//**************************************************************************************

void I_CreateBackBuffer_e32()
{	
	I_CreateWindow_e32();
}

//**************************************************************************************

void I_FinishUpdate_e32(const uint8_t* srcBuffer, const uint8_t* pallete, const unsigned int width UNUSED, const unsigned int height UNUSED)
{
    // BDPNOTE: This is where the screenbuffer is drawn
    pb = (unsigned char*)srcBuffer;
    pl = (unsigned char*)pallete;

    static int filenum = 0;
    static uint32_t timebase = 0xffffffff;
    char filename[256];
    snprintf(filename, sizeof(filename), "screenbuffers/scr%05d.raw", filenum++);
    struct image_header_s
    {
        uint32_t sequence_no;
        uint32_t timestamp_ms;
        uint16_t width;
        uint16_t height;
        struct {
            uint8_t r;
            uint8_t g;
            uint8_t b;
        } palette[256] __attribute__((packed));
    } header;
    #ifdef __chess__
    unsigned cyclecount = chess_cycle_count();
    printf("Writing file %d after %u cycles\n",filenum,cyclecount);
    #endif
    
    FILE *f = fopen(filename, "wb");
    if (f) {
        #ifndef __chess__
        clock_t clock_now = clock();
        uint32_t time_ms = (uint32_t)((double)clock_now / ((double)CLOCKS_PER_SEC / 1000.0));
        #else
        uint32_t clock_now = chess_cycle_count() >> 5;
        uint32_t time_ms = clock_now/1000;
        #endif
        if (timebase == 0xffffffff)
            timebase = time_ms;
        header.timestamp_ms = time_ms-timebase;
        header.sequence_no = filenum;
        header.width = 2*width;
        header.height = height;
        for (int i=0; i<256; i++) {
            header.palette[i].r = pl[3*i];
            header.palette[i].g = pl[3*i+1];
            header.palette[i].b = pl[3*i+2];
        }
        fwrite(&header, sizeof(header), 1, f);
        fwrite(srcBuffer, 2*width*height, 1, f);
        fclose(f);
    } else {
        printf("Failed to open screenbuffer dump file %s\n", filename);
    }

    // Stop after ~10 seconds
    if (filenum == 350) {
        printf("\n\n.. It did run DOOM\n");
        exit(0);
    }
}

//**************************************************************************************

void I_SetPallete_e32(CachedBuffer<uint8_t> pallete UNUSED)
{

}

//**************************************************************************************

int I_GetVideoWidth_e32()
{
    return vid_width;
}

//**************************************************************************************

int I_GetVideoHeight_e32()
{
	return vid_height;
}

//**************************************************************************************

void I_ProcessKeyEvents()
{
	I_PollWServEvents_e32();
}

//**************************************************************************************

#define MAX_MESSAGE_SIZE 128

extern "C"
void I_Error (const char *error, ...)
{
	char msg[MAX_MESSAGE_SIZE+1];
 
    va_list v;
    va_start(v, error);

    int n = vsnprintf(msg, MAX_MESSAGE_SIZE, error, v);

    va_end(v);

    if (n < 0)
    {
        msg[0] = '\0';
    }
    else if ((size_t)n >= MAX_MESSAGE_SIZE)
    {
        msg[MAX_MESSAGE_SIZE - 1] = '\0';
    }

    printf("%s\n", msg);


    fflush( stderr );
	fflush( stdout );

    //fgets(msg, sizeof(msg), stdin);

	I_Quit_e32();
}

//**************************************************************************************

void I_Quit_e32()
{

}

//**************************************************************************************
