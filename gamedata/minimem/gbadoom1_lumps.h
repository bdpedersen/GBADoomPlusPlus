#ifndef _GBADOOM1_LUMPS_H_
#define _GBADOOM1_LUMPS_H_

#include <stdint.h>
#include "annotations.h"

#define WADLUMPS 1158


extern ConstMemArray<int32_t> filepos;
extern ConstMemArray<int32_t> lumpsize;
extern ConstMemArray<uint32_t> lumpname_high;
extern ConstMemArray<uint32_t> lumpname_low;

#endif // _GBADOOM1_LUMPS_H_
