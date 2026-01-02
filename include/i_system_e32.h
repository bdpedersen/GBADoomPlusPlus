// PsionDoomApp.h
//
// Copyright 17/02/2019 
//

#ifndef HEADER_ISYSTEME32
#define HEADER_ISYSTEME32

#include "../newcache/newcache.h"

#ifdef __cplusplus
extern "C" {
#endif


void I_InitScreen_e32();

void I_CreateBackBuffer_e32();

int I_GetVideoWidth_e32();

int I_GetVideoHeight_e32();

void I_FinishUpdate_e32(const uint8_t* srcBuffer, const uint8_t* pallete, const unsigned int width, const unsigned int height);

void I_SetPallete_e32(CachedBuffer<uint8_t> pallete);

void I_ProcessKeyEvents();

int I_GetTime_e32(void);

void I_Quit_e32();

unsigned short* I_GetBackBuffer();

unsigned short* I_GetFrontBuffer();

void I_Error (const char *error, ...);

#ifdef __cplusplus
}
#endif



#endif
