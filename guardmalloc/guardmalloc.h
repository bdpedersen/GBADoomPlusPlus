#ifndef __guardmalloc_h__
#define __guardmalloc_h__
// This library dfines guardmalloc, a debugging malloc library
// that helps detect memory overwrites and leaks.
// It does this by allocating extra "guard" pages before
// and after each allocation, and by keeping track of
// all allocated blocks. 
// Allocation is done using anonymous mmap() to get page-aligned
// memory regions, and mprotect() to set guard pages
// as inaccessible. Any access to these guard pages
// will cause a segmentation fault, which can be caught
// using a debugger to find the source of the memory error.
// Free will not free the pages, but will read and write 
// protect them to catch any use-after-free errors.
// This library is intended for use in debugging and testing,
// and should not be used in production code due to its
// performance overhead and increased memory usage. It is 
// built using standard POSIX system calls, so it should be
// portable to any POSIX-compliant operating system.
#include <stdlib.h>
void *gmalloc(size_t size, const char *file, int line);
void gfree(void *ptr, const char *file, int line);

#define GMALLOC(size) gmalloc(size, __FILE__, __LINE__)
#define GFREE(ptr) gfree(ptr, __FILE__, __LINE__)

void gcheckleaks();
void gflushfreed();
int ggetnumfreed();
size_t getpendingfreesize();

#endif // __guardmalloc_h__