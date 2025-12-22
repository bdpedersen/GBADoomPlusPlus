#ifndef __NEWCACHE_H__
#define __NEWCACHE_H__  

 /* Emacs style mode select   -*- C++ -*-
 *-----------------------------------------------------------------------------
 *
 *
 *  PrBoom: a Doom port merged with LxDoom and LSDLDoom
 *  based on BOOM, a modified and improved DOOM engine
 *  Copyright (C) 1999 by
 *  id Software, Chi Hoang, Lee Killough, Jim Flynn, Rand Phares, Ty Halderman
 *  Copyright (C) 1999-2000 by
 *  Jess Haas, Nicolas Kalkhof, Colin Phipps, Florian Schulze
 *  Copyright 2005, 2006 by
 *  Florian Schulze, Colin Phipps, Neil Stevens, Andrey Budko
 *  Copyright 2025 by
 *  Brian Dam Pedersen
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 *  02111-1307, USA.
 *
 * DESCRIPTION:
 *      New caching system interface. Portions from original w_wad.h
 *
 *-----------------------------------------------------------------------------*/

#include <stdint.h>


const void * NC_CacheLumpNum(int lumpnum);
int NC_GetNumForName (const char* name);
int NC_CheckNumForName(const char *name);
const char* NC_GetNameForNum(int lump);
int NC_LumpLength(int lumpnum);
void NC_Init(void);
void NC_ExtractFileBase(const char* path, char* dest);

// WAD parser types
typedef struct
{
  char identification[4];                  // Should be "IWAD" or "PWAD".
  int  numlumps;
  int  infotableofs;
} wadinfo_t;

typedef struct
{
  int  filepos;
  int  size;
  char name[8];
} filelump_t;


template <typename T>
class Cached;

#define STBAR_LUMP_NUM -2

extern unsigned char gfx_stbar[];


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
        CachedBuffer(const char* name) : lumpnum(NC_GetNumForName(name)), _byteoffset(0) {}

        const Cached<T> operator[](int index) const {
            return Cached<T>(lumpnum, _byteoffset+index*sizeof(T));
        }
        
        int size() const {
            return (NC_LumpLength(lumpnum)-_byteoffset) / sizeof(T);
        }

        template <typename U>
        CachedBuffer<U> transmute() const  {
            return CachedBuffer<U>(lumpnum,_byteoffset);
        }   

        CachedBuffer<T> addOffset(int offset) const {
            return CachedBuffer<T>(lumpnum, _byteoffset+offset*sizeof(T));
        }

        Pinned<T> pin() const {
            return Pinned<T>((const T*)(base() + _byteoffset), lumpnum);
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
            return *(T*)(base() + _byteoffset);
        }

        CachedBuffer<T>& operator+=(int count) {
            _byteoffset += count * sizeof(T);
            return *this;
        }

    private:
        const char * base() const {
            return (const char *)NC_CacheLumpNum(lumpnum);
        }

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
        Cached(const char* name) : lumpnum(NC_GetNumForName(name)), byteoffset(0) {}

        const Sentinel<T> operator->() const {
            
            return Sentinel<T>((const T*)(base() + byteoffset), lumpnum);
        }

        T operator*() const {
            return *(T*)(base() + byteoffset);
        }

        const Pinned<T> pin() const {
            return Pinned<T>((const T*)(base() + byteoffset), lumpnum);
        }

        template <typename U>
        Cached<U> transmuteToObjectAtByteOffset(int extrabyteoffset) const  {
            return Cached<U>(lumpnum, byteoffset+extrabyteoffset);
        }

        CachedBuffer<uint8_t> bytebuffer() {
            return CachedBuffer<uint8_t>(lumpnum,byteoffset);
        }
        Cached operator++(int) {
            Cached<T> old = *this;
            byteoffset += sizeof(T);
            return old;
        }   
    private:
        const char * base() const {
            // TODO: Address this by pemanently pinning an entry in the cache for this
            if (lumpnum == STBAR_LUMP_NUM){
                return (const char *)gfx_stbar; // Violent hack !
            }
            return (const char *)NC_CacheLumpNum(lumpnum);
        }
        short lumpnum;
        unsigned int byteoffset;
};



#endif // __NEWCACHE_H__