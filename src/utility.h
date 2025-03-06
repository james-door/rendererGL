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
    assert(file_stream.is_open() && "Invalid Path"); 
    file_stream.seekg(0,std::ios_base::end);
    i32 size = static_cast<i32>(file_stream.tellg());
    assert(size > 0);
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
#define RENDERER_LOG(msg) rendererLogConsole(msg)


inline void rendererLogConsole(const char * msg)
{
    constexpr i32 max_timed_log_buffer = 50;
    char time_log_buffer[max_timed_log_buffer];
    time_t t = time(NULL);
    struct tm *tm_info = localtime(&t);
    i32 data_len = strftime(time_log_buffer, max_timed_log_buffer, "%Y-%m-%d %H:%M:%S", tm_info);

    assert(data_len != 0);
    std::cout<<time_log_buffer<<" :: "<<msg<<'\n';
}


#endif