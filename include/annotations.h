#ifndef __annotations_h
#define __annotations_h

#define UNUSED __attribute__((unused))

#ifndef GBA
#define MAYBE_UNUSED UNUSED
#else
#define MAYBE_UNUSED
#endif


#endif /* annotations.h */

