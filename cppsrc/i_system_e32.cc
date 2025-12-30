// PsionDoomDoc.cpp
//
// Copyright 17/02/2019 
//

#ifndef GBA


#include "i_system_win.h"

#include <stdarg.h>
#include <stdio.h>

#include "i_system_e32.h"

#include "lprintf.h"

#include "annotations.h"

#ifdef DUMP_SCREENBUFFER
#include <sys/stat.h>
#include <sys/types.h>
#endif

//**************************************************************************************

unsigned int vid_width = 0;
unsigned int vid_height = 0;

unsigned int screen_width = 0;
unsigned int screen_height = 0;

unsigned int y_pitch = 0;

DoomWindow* window = NULL;

QApplication * app = NULL;

unsigned char* pb = NULL;
unsigned char* pl = NULL;


unsigned char* thearray = NULL;
int thesize;

unsigned short backbuffer[120 *160];
unsigned short frontbuffer[120 *160];

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
    int z = 0;

    app = new QApplication (z, nullptr);

    window = new DoomWindow();

    #ifndef __APPLE__
    window->setAttribute(Qt::WA_PaintOnScreen);
    #endif


    window->resize(vid_width * 8, vid_height * 4);

    window->show();
}

//**************************************************************************************

void I_CreateBackBuffer_e32()
{	
	I_CreateWindow_e32();
    #ifdef DUMP_SCREENBUFFER
    // Create directory to put the screen buffer files into - use posix for this.
    // Only create this if it doesn't already exist.
    struct stat st;
    if (!(stat("screenbuffers", &st) == 0 && S_ISDIR(st.st_mode)))
    {
        mkdir("screenbuffers", S_IRWXU | S_IRGRP | S_IROTH | S_IXOTH);
    }
    #endif
}

//**************************************************************************************

void I_FinishUpdate_e32(const byte* srcBuffer, const byte* pallete, const unsigned int width UNUSED, const unsigned int height UNUSED)
{
    // BDPNOTE: This is where the screenbuffer is drawn
    pb = (unsigned char*)srcBuffer;
    pl = (unsigned char*)pallete;

    window->repaint();

    app->processEvents();

    /*
    int arrayCount = thesize;

    if(arrayCount == 0)
        return;

    //dump the _g->viewangletox var
    QFile f("C:\\temp\\gfx_stbar.c");
    f.open(QIODevice::ReadWrite);

    f.write("const byte gfx_stbar[");
    f.write(QString::number(arrayCount).toLatin1().constData());

    f.write("] =\n{\n");

    for(int i = 0; i < arrayCount; i++)
    {
        f.write(QString::number(thearray[i]).toLatin1().constData());
        f.write(",");

        if((i%16) == 0)
            f.write("\n");
    }

    f.write("\n};\n");

    f.close();
    */
   #ifdef DUMP_SCREENBUFFER
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
    FILE *f = fopen(filename, "wb");
    if (f) {
        clock_t clock_now = clock();
        uint32_t time_ms = (uint32_t)((double)clock_now / ((double)CLOCKS_PER_SEC / 1000.0));
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
   #endif
}

//**************************************************************************************

void I_SetPallete_e32(CachedBuffer<byte> pallete UNUSED)
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

#define MAX_MESSAGE_SIZE 1024

extern "C"
void I_Error (const char *error, ...)
{
	char msg[MAX_MESSAGE_SIZE];
 
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

#endif
