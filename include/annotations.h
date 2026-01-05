#ifndef __annotations_h
#define __annotations_h

#define UNUSED __attribute__((unused))

#ifndef GBA
#define MAYBE_UNUSED UNUSED
#else
#define MAYBE_UNUSED
#endif

#ifdef __chess__
#define CONSTMEMAREA chess_storage(PMEM)
#define CONSTMEM chess_storage(PMEM%2)
#else
#define CONSTMEMAREA
#define CONSTMEM
#endif

// Slightly ugly hack allowing address translation for constmem access
template <typename T> 
struct ConstMemArray {
public:
    ConstMemArray(const T CONSTMEMAREA *ptr ) : data(ptr) {
        static_assert(sizeof(T)==4);
    }

    // Read-only accessor
    const T operator[](size_t index) const {
        
        uintptr_t ptr = (uintptr_t)&data[index];
        #ifdef XLATADDR
        ptr >>= 1;
        #endif
        return *((uint32_t CONSTMEMAREA *)ptr);  
    }
    
private:
    const T CONSTMEMAREA *data;  // must be public for aggregate initialization

};


#endif /* annotations.h */

