#ifndef BASETYPES_H
#define BASETYPES_H

#include <xmmintrin.h>
#include <cstdint>

#define ENABLE_ASSERTS
//#define ENABLE_EMULATION_MESSAGEBOXES // also enables "DumpCompiledBlocks" option

#ifdef WIN32

#define STRICT
#define WIN32_LEAN_AND_MEAN

#include <windows.h>

#define FASTCALL_START __fastcall
#define FASTCALL_END

#else //WIN32

typedef unsigned long GUID;
typedef long LONG;
typedef void *HWND;

#define MAX_PATH 260

#define VK_RIGHT 1
#define VK_LEFT 2
#define VK_UP 3
#define VK_DOWN 4

// Replace windows only _countof
#include <cstddef>
template <typename T, std::size_t N>
constexpr std::size_t _countof(const T (&arr)[N])
{
  return N;
}

unsigned long _lrotl(unsigned long __X, int __C)
{
  return (__X << __C) | (__X >> ((sizeof(long) * 8) - __C));
}

unsigned long _lrotr(unsigned long __X, int __C)
{
  return (__X >> __C) | (__X << ((sizeof(long) * 8) - __C));
}

int strcpy_s(char *dest, size_t destsz, const char *src);
int strcat_s(char *dest, size_t destsz, const char *src);
int sprintf_s(char *dest, size_t destsz, const char *format, ...);

char* strtok_s(char* str, const char* delim, char** context);

int memmove_s(void* dest, size_t destSize, const void* src, size_t count);

#define __popcnt __builtin_popcount

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;
typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;
typedef unsigned __int128 uint128;
typedef unsigned char uchar;
typedef signed char schar;

#define FASTCALL_START
#define FASTCALL_END __attribute__((fastcall))

#ifndef O_BINARY
#define O_BINARY 0
#endif
#ifndef O_TEXT
#define O_TEXT 0
#endif

#endif  //WIN32

#if 1 // initialize all nuon memory to a defined value (0xcd, like VS debugger)
#include <memory.h>
inline void init_nuon_mem(uint8* const p, const size_t length)
{
  memset(p,0xcd,length);
}
#else
inline void init_nuon_mem(const uint8* const p, const size_t length) {}
#endif

#ifdef ENABLE_ASSERTS
#include <cassert>
#else
#define assert(expression) ((void)0)
#endif

#endif
