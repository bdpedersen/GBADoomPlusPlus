#ifndef _GBADOOM1_LUMPS_H_
#define _GBADOOM1_LUMPS_H_

#include <stdint.h>
#include "annotations.h"

#define WADLUMPS 1158

extern int32_t CONSTMEM filepos[WADLUMPS];
extern int32_t CONSTMEM lumpsize[WADLUMPS];
extern uint32_t CONSTMEM lumpname_high[WADLUMPS];
extern uint32_t CONSTMEM lumpname_low[WADLUMPS];

#endif // _GBADOOM1_LUMPS_H_
