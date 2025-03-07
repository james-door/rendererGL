



#ifndef UTILITY_H
#define UTILITY_H

#include "defintions.h"
#include "arena.h"
#include "string_view"
#include <fstream>
#include <cassert>






std::string_view loadFile(StackArena &arena, char* path)
{
    std::ifstream file_stream {path, std::ifstream::binary};
    
    RENDERER_ASSERT(file_stream.is_open(), "Invalid Path."); 
    
    file_stream.seekg(0,std::ios_base::end);
    i32 size = static_cast<i32>(file_stream.tellg());
    RENDERER_ASSERT(file_stream.is_open(), "Empty File."); 
    file_stream.seekg(0,std::ios_base::beg);
    
    char* buffer = arena.arenaPush<char>(size);
    file_stream.read(buffer, size);
    return std::string_view{buffer,static_cast<u64>(size)};
}


u32 stringToU32(const char* str, u8 length)
{
    assert(length <= 10 && length > 0);
    u32 result = 0;
    for(u8 i = 0; i < length; ++i)
    {
        assert('0' <= str[i] && '9' >= str[i]);
        result = result * 10 + (str[i] - '0');
    }
    return result;
}

// should log to file

// for release
// #define RENDERER_LOG(msg) ((void)0)



#endif