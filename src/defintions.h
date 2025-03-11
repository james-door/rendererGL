#ifndef DEFINTIONS_H
#define DEFINTIONS_H

#include <iostream>
#include <time.h>
#include <fstream>
#include <cstdarg>
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;

typedef signed char i8;
typedef signed short i16;
typedef signed int i32;
typedef signed long long i64;

typedef float f32;
typedef double f64;

#if defined(__clang__) || defined(__gcc__)
#define STATIC_ASSERT _Static_assert
#else
#define STATIC_ASSERT static_assert //MSVC
#endif

STATIC_ASSERT(sizeof(u8) == 1, "Expected u8 to be 1 byte.");
STATIC_ASSERT(sizeof(u16) == 2, "Expected u16 to be 2 bytes.");
STATIC_ASSERT(sizeof(u32) == 4, "Expected u32 to be 4 bytes.");
STATIC_ASSERT(sizeof(u64) == 8, "Expected u64 to be 8 bytes.");

STATIC_ASSERT(sizeof(i8) == 1, "Expected i8 to be 1 byte.");
STATIC_ASSERT(sizeof(i16) == 2, "Expected i16 to be 2 bytes.");
STATIC_ASSERT(sizeof(i32) == 4, "Expected i32 to be 4 bytes.");
STATIC_ASSERT(sizeof(i64) == 8, "Expected i64 to be 8 bytes.");

STATIC_ASSERT(sizeof(f32) == 4, "Expected f32 to be 4 bytes.");
STATIC_ASSERT(sizeof(f64) == 8, "Expected f64 to be 8 bytes."); 




void getTime(char *buffer, i32 max_length)
{

    time_t t = time(NULL);
    tm *tm_info = localtime(&t);
    strftime(buffer, max_length, "%Y-%m-%d %H:%M:%S", tm_info);
}

inline void rendererLogConsole(const char *format, ...) {
    constexpr size_t max_log_buffer = 1024;
    char log_buffer[max_log_buffer];

    va_list args;
    va_start(args, format);
    vsnprintf(log_buffer, max_log_buffer, format, args);
    va_end(args);

    constexpr size_t max_timed_log_buffer = 50;
    char time_log_buffer[max_timed_log_buffer];
    getTime(time_log_buffer, max_timed_log_buffer);

    std::cout << time_log_buffer << " :: " << log_buffer << '\n';
}

inline void rendererLogFile(const char *format, ...)
{
    constexpr char dump_path[] = "log.txt";
    constexpr size_t max_log_buffer = 1024;
    char log_buffer[max_log_buffer];

    va_list args;
    va_start(args, format);
    vsnprintf(log_buffer, max_log_buffer, format, args);
    va_end(args);

    constexpr size_t max_timed_log_buffer = 50;
    char time_log_buffer[max_timed_log_buffer];
    getTime(time_log_buffer, max_timed_log_buffer);

    std::ofstream out{dump_path, std::ofstream::app};  
    out<<time_log_buffer<<" :: "<<log_buffer<<'\n';
    out.close();
}
#define RENDERER_LOG(...) rendererLogConsole(__VA_ARGS__)

void logFileIfFailed(bool err, const char* file, int line, const char *format, ...)
{
    if (!err)
    {
        constexpr char dump_path[] = "log.txt";
        constexpr size_t max_log_buffer = 1024;
        char log_buffer[max_log_buffer];

        va_list args;
        va_start(args, format);
        vsnprintf(log_buffer, max_log_buffer, format, args);
        va_end(args);

        constexpr size_t max_timed_log_buffer = 50;
        char time_log_buffer[max_timed_log_buffer];
        getTime(time_log_buffer, max_timed_log_buffer);


        std::ofstream out{dump_path, std::ofstream::app};  
        out<<time_log_buffer<<" :: "<<log_buffer<<" :: "<<file<<" :: "<<line<<'\n';
        out.close();

        RENDERER_LOG("Aborting after failed assert.");
        abort();
    }
}
void logConsoleIfFailed(bool err, const char* file, int line, const char *format, ...)
{
    if (!err)
    {
        constexpr size_t max_log_buffer = 1024;
        char log_buffer[max_log_buffer];

        va_list args;
        va_start(args, format);
        vsnprintf(log_buffer, max_log_buffer, format, args);
        va_end(args);

        constexpr size_t max_timed_log_buffer = 50;
        char time_log_buffer[max_timed_log_buffer];
        getTime(time_log_buffer, max_timed_log_buffer);

        std::cerr<<time_log_buffer<<" :: "<<log_buffer<<" :: "<<file<<" :: "<<line<<'\n';

        RENDERER_LOG("Aborting after failed assert.");
        abort();
    }
}
#define RENDERER_ASSERT(err, ...) logConsoleIfFailed(err, __FILE__, __LINE__, __VA_ARGS__)


#endif