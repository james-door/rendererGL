#ifndef DEFINTIONS_H
#define DEFINTIONS_H


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

inline void rendererLogConsole(const char * msg)
{
    constexpr i32 max_timed_log_buffer = 50;
    char time_log_buffer[max_timed_log_buffer];
    getTime(time_log_buffer, max_timed_log_buffer);
    std::cout<<time_log_buffer<<" :: "<<msg<<'\n';
}

inline void rendererLogFile(const char * msg)
{
    constexpr char dump_path[] = "log.txt";
    constexpr i32 max_timed_log_buffer = 50;
    char time_log_buffer[max_timed_log_buffer];

    getTime(time_log_buffer, max_timed_log_buffer);

    std::ofstream out{dump_path, std::ofstream::app};  
    out<<time_log_buffer<<" :: "<<msg<<'\n';
    out.close();
}
#define RENDERER_LOG(msg) rendererLogConsole(msg)

void logFileIfFailed(bool err, const char* msg, const char* file, int line)
{
    if (!err)
    {
        constexpr char dump_path[] = "log.txt";
        constexpr i32 max_timed_log_buffer = 50;
        char time_log_buffer[max_timed_log_buffer];
        
        getTime(time_log_buffer, max_timed_log_buffer);

        std::ofstream out{dump_path, std::ofstream::app};  
        out<<time_log_buffer<<" :: "<<msg<<" :: "<<file<<" :: "<<line<<'\n';
        out.close();

        RENDERER_LOG("Aborting after failed assert.");
        abort();
    }
}
void logConsoleIfFailed(bool err, const char* msg, const char* file, int line)
{
    if (!err)
    {
        constexpr i32 max_timed_log_buffer = 50;
        char time_log_buffer[max_timed_log_buffer];
        
        getTime(time_log_buffer, max_timed_log_buffer);

        std::cerr<<time_log_buffer<<" :: "<<msg<<" :: "<<file<<" :: "<<line<<'\n';

        RENDERER_LOG("Aborting after failed assert.");
        abort();
    }
}
#define RENDERER_ASSERT(err, msg) logConsoleIfFailed(err, msg, __FILE__, __LINE__)


#endif