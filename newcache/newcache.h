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

        bool isnull() const {
            return ptr == nullptr;
        }

    private:
        const T* ptr;
        short lumpnum;
};

template <typename T>
class CachedBuffer {
    public:
        CachedBuffer() : lumpnum(-1), _byteoffset(0) {}
        CachedBuffer(short lumpnum) : lumpnum(lumpnum), _byteoffset(0) {} 
        CachedBuffer(short lumpnum, unsigned int byteoffset) : lumpnum(lumpnum), _byteoffset(byteoffset) {}
        CachedBuffer(const char* name) : lumpnum(W_GetNumForName(name)), _byteoffset(0) {}

        const Cached<T> operator[](int index) const {
            return Cached<T>(lumpnum, _byteoffset+index*sizeof(T));
        }
        
        int size() const {
            return (W_LumpLength(lumpnum)-_byteoffset) / sizeof(T);
        }

        template <typename U>
        CachedBuffer<U> transmute() const  {
            return CachedBuffer<U>(lumpnum,_byteoffset);
        }   

        CachedBuffer<T> addOffset(int offset) const {
            return CachedBuffer<T>(lumpnum, _byteoffset+offset*sizeof(T));
        }

        Pinned<T> pin() const {
            const char * base = (const char*)W_CacheLumpNum(lumpnum);
            return Pinned<T>((const T*)(base + _byteoffset), lumpnum);
        }   

        bool isnull() const {
            return lumpnum == -1;
        }

        bool isvalid() const {
            return lumpnum != -1;
        }

        unsigned int byteoffset() const {
            return _byteoffset;
        }

        CachedBuffer<T> operator++(int) {
            CachedBuffer<T> old = *this;
            _byteoffset += sizeof(T);
            return old;
        }

        T operator*() const {
            const char * base = (const char*)W_CacheLumpNum(lumpnum);
            return *(T*)(base + _byteoffset);
        }

        CachedBuffer<T>& operator+=(int count) {
            _byteoffset += count * sizeof(T);
            return *this;
        }

    private:
        short lumpnum;
        unsigned int _byteoffset;
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

        const Pinned<T> pin() const {
            const char * base = (const char*)W_CacheLumpNum(lumpnum);
            return Pinned<T>((const T*)(base + byteoffset), lumpnum);
        }

        template <typename U>
        Cached<U> transmuteToObjectAtByteOffset(int extrabyteoffset) const  {
            return Cached<U>(lumpnum, byteoffset+extrabyteoffset);
        }

        CachedBuffer<byte> bytebuffer() {
            return CachedBuffer<byte>(lumpnum,byteoffset);
        }

        Cached operator++(int) {
            Cached<T> old = *this;
            byteoffset += sizeof(T);
            return old;
        }   

    private:
        short lumpnum;
        unsigned int byteoffset;
};



#endif // __NEWCACHE_H__