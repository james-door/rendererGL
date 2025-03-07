#ifndef ARENA_H
#define ARENA_H

#include "defintions.h"
#include <limits>
#include <cassert>
#include <cstring>
#include <cstddef>



template <typename T>
inline T* arenaPushImpl(i8* start, ptrdiff_t& offset, i8* end, u64 nItems, bool zeroInit)
{
    ptrdiff_t size = sizeof(T) * nItems;
    ptrdiff_t align = alignof(T);    
    ptrdiff_t arena_head = std::bit_cast<ptrdiff_t, i8*>(start + offset);
    ptrdiff_t padding = (align - arena_head % align) % align;

    void* new_arena_head = static_cast<void*>(start + offset + padding);

    offset += size + padding;
    RENDERER_ASSERT((std::bit_cast<ptrdiff_t, i8*>(end) - (arena_head + padding + size)) >= 0, "Arena ran out of memory.");
    RENDERER_ASSERT((std::bit_cast<ptrdiff_t, void*>(new_arena_head) % align) == 0, "Somehow broke alignment.");


    if (zeroInit) memset(new_arena_head, 0, size);
    return new(new_arena_head) T[nItems]; // avoid UB by actually creating an object
}

struct Arena
{
    i8* start;
    ptrdiff_t offset;
    i8* end;
    Arena() : start{nullptr}, offset{0}, end{nullptr} {}
    Arena(ptrdiff_t size) : start{new i8[size]}, offset{0}, end{start + size} {}
    void allocate(ptrdiff_t size) { assert(!start); start = new i8[size]; end = start + size; offset = 0; }
    void freeArena() { delete[] start; }
    template <typename T> T* arenaPush(u64 nItems) { return arenaPushImpl<T>(start, offset, end, nItems, false); }
    template <typename T> T* arenaPushZero(u64 nItems) { return arenaPushImpl<T>(start, offset, end, nItems, true); }
};

struct LifeTimeArena
{
    i8* start;
    ptrdiff_t offset;
    i8* end;
    LifeTimeArena() : start{nullptr}, offset{0}, end{nullptr} {}
    LifeTimeArena(ptrdiff_t size) : start{new i8[size]}, offset{0}, end{start + size} {}
    template <typename T> T* arenaPush(u64 nItems) { return arenaPushImpl<T>(start, offset, end, nItems, false); }
    template <typename T> T* arenaPushZero(u64 nItems) { return arenaPushImpl<T>(start, offset, end, nItems, true); }
};

struct StackArena
{
    i8* start;
    ptrdiff_t offset;
    i8* end;
    StackArena(ptrdiff_t size) : start{new i8[size]}, offset{0}, end{start + size} {}
    ~StackArena() { delete[] start; }
    template <typename T> T* arenaPush(u64 nItems) { return arenaPushImpl<T>(start, offset, end, nItems, false); }
};

// same as a span the SubArena has no owernship over the data life-time
struct SubArena
{
    i8* start;
    ptrdiff_t offset;
    i8* end;

    SubArena() : start{nullptr}, offset{0}, end{nullptr} {}
    SubArena(Arena &arena, ptrdiff_t size)          {init<Arena>(arena,size);}
    SubArena(StackArena &arena, ptrdiff_t size)     {init<StackArena>(arena,size);}
    SubArena(LifeTimeArena &arena, ptrdiff_t size)  {init<LifeTimeArena>(arena,size);}

    template <typename ArenaType>
    inline void init(ArenaType &arena, ptrdiff_t size)
    {
        ptrdiff_t arena_head = std::bit_cast<ptrdiff_t, i8*>(arena.start) + arena.offset;
        ptrdiff_t padding = (alignof(std::max_align_t) -  arena_head % alignof(std::max_align_t)) % alignof(std::max_align_t);
        
        arena.template arenaPush<i8>(padding);
        start = arena.template arenaPush<i8>(size);
        
        end = arena.start + arena.offset;
        offset = 0; 
    }

    template <typename T> 
    T* arenaPush(u64 nItems)
    { 
        return arenaPushImpl<T>(start, offset, end, nItems, false);
    }

    template <typename T> 
    T* arenaPushZero(u64 nItems)
    {
         return arenaPushImpl<T>(start, offset, end, nItems, true);
    }
};

#endif