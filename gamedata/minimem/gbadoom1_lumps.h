#ifndef _GBADOOM1_LUMPS_H_
#define _GBADOOM1_LUMPS_H_

#include <stdint.h>

#define WADLUMPS 1158

extern int32_t filepos[WADLUMPS];
extern int32_t lumpsize[WADLUMPS];
extern uint32_t lumpname_high[WADLUMPS];
extern uint32_t lumpname_low[WADLUMPS];

#endif // _GBADOOM1_LUMPS_H_
