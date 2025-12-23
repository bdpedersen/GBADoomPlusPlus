#ifndef __wadreader_h
#define __wadreader_h

#include <stdint.h>

void WR_Init();
void WR_Read(uint8_t *dst, int offset, int len);

#endif //__wadreader_h