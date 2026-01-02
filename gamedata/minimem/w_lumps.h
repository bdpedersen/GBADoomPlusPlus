#ifndef __W_LUMPS_H__
#define __W_LUMPS_H__
#include <stdint.h>
#include "gbadoom1_lumps.h"
#include "../newcache/newcache.h"

int LC_CheckNumForName(const char *name);
const char* LC_GetNameForNum(int lump, char buffer[8]);
int LC_LumpLength(int lumpnum);
filelump_t LC_LumpForNum(int lumpnum);
void LC_Init(void);
 


#endif /* __W_LUMPS_H__ */