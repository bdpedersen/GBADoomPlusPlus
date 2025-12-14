#ifndef __NEWCACHE_H__
#define __NEWCACHE_H__  

const void * W_CacheLumpNum(int lumpnum);
int W_LumpLength(int lumpnum);
int W_GetNumForName (const char* name);
int W_CheckNumForName(const char *name);

template <typename T>
class Cached;

template <typename T>
class Pinned {
    public:
    // TODO: implement pinning mechanism
        Pinned() : ptr(nullptr), lumpnum(-1) {}
        Pinned(const T* ptr, short lumpnum) : ptr(ptr), lumpnum(lumpnum) {}
        ~Pinned() {}

        operator const T*() const {
            return ptr;
        }

    private:
        const T* ptr;
        short lumpnum;
};

template <typename T>
class CachedBuffer {
    public:
        CachedBuffer() : lumpnum(-1), byteoffset(0) {}
        CachedBuffer(short lumpnum) : lumpnum(lumpnum), byteoffset(0) {} 
        CachedBuffer(short lumpnum, unsigned int byteoffset) : lumpnum(lumpnum), byteoffset(byteoffset) {}
        CachedBuffer(const char* name) : lumpnum(W_GetNumForName(name)), byteoffset(0) {}

        const Cached<T> operator[](int index) {
            return Cached<T>(lumpnum, index*sizeof(T));
        }

        int size() {
            return (W_LumpLength(lumpnum)-byteoffset) / sizeof(T);
        }

        template <typename U>
        CachedBuffer<U> transmute() {
            return CachedBuffer<U>(lumpnum,byteoffset);
        }   

        CachedBuffer<T> addOffset(int offset) {
            return CachedBuffer<T>(lumpnum, byteoffset+offset*sizeof(T));
        }

        Pinned<T> pin() {
            const char * base = (const char*)W_CacheLumpNum(lumpnum);
            return Pinned<T>((const T*)(base + byteoffset), lumpnum);
        }   

    private:
        short lumpnum;
        unsigned int byteoffset;
};

template <typename T>
class Sentinel {
    public:
    // TODO: implement pinning mechanism
        Sentinel() : ptr(nullptr), lumpnum(-1) {}
        Sentinel(const T* ptr, short lumpnum) : ptr(ptr), lumpnum(lumpnum) {}
        ~Sentinel() {}

        const T* operator->() const {
            return ptr;
        }

    private:
        const T* ptr;
        short lumpnum;
};

template <typename T>
class Cached {
    public:
        Cached() : lumpnum(-1), byteoffset(0) {}
        Cached(short lumpnum) : lumpnum(lumpnum), byteoffset(0) {}
        Cached(short lumpnum, int offset) : lumpnum(lumpnum), byteoffset(offset) {}
        Cached(const char* name) : lumpnum(W_GetNumForName(name)), byteoffset(0) {}

        const Sentinel<T> operator->() const {
            
            const char * base = (const char*)W_CacheLumpNum(lumpnum);
            return Sentinel<T>((const T*)(base + byteoffset), lumpnum);
        }

        T value() const {
            const char * base = (const char*)W_CacheLumpNum(lumpnum);
            return *(T*)(base + byteoffset);
        }

        CachedBuffer<T> buffer() {
            return CachedBuffer<T>(lumpnum,byteoffset);
        }

    private:
        short lumpnum;
        unsigned int byteoffset;
};



#endif // __NEWCACHE_H__