#ifndef BYTESWAP_H
#define BYTESWAP_H

#define LITTLE_ENDIAN

#include "basetypes.h"
#include <intrin.h>

#ifdef LITTLE_ENDIAN

#ifdef __GNUC__
#define __forceinline inline __attribute__((always_inline))

#include <cstdint>
#include <bit>

uint16_t _byteswap_ushort(uint16_t value)
{
  return (value & 0xFF00) >> 8 | (value & 0x00FF) << 8;
}

uint64_t _byteswap_ulong(uint64_t value)
{
  return (value & 0xFFFFFFFF00000000) >> 32 | (value & 0x00000000FFFFFFFF) << 32;
}

// STD C++23
//unsigned long _byteswap_ulong(unsigned long value)
//{
//  return std::byteswap(value);
//}

#endif

    __forceinline void
    SwapWordBytes(uint16 *const toswap)
{
#if 0 // old __fastcall dependent code
  __asm
  {
    mov ax, [ecx]
    xchg ah, al
    mov [ecx], ax
  }
#else
  *toswap = _byteswap_ushort(*toswap);
#endif
}

__forceinline uint16 SwapBytes(const uint16 toswap)
{
  return _byteswap_ushort(toswap);
}

__forceinline void SwapScalarBytes(uint32 * const toswap)
{
#if 0 // old __fastcall dependent code
  __asm
  {
    mov eax, [ecx]
    bswap eax
    mov [ecx], eax
  }
#else
  *toswap = _byteswap_ulong(*toswap);
#endif
}

__forceinline uint32 SwapBytes(const uint32 toswap)
{
  return _byteswap_ulong(toswap);
}

__forceinline void SwapShortVectorBytes(uint16 toswap[4])
{
#if 0 // old __fastcall dependent code
  __asm
  {
    mov dx, [ecx]
    mov ax, [ecx + 2]
    xchg dl, dh
    xchg al, ah
    mov [ecx], dx
    mov [ecx + 2], ax
    mov dx, [ecx + 4]
    mov ax, [ecx + 6]
    xchg dl, dh
    xchg al, ah
    mov [ecx + 4], dx
    mov [ecx + 6], ax
  }
#else
  toswap[0] = _byteswap_ushort(toswap[0]);
  toswap[1] = _byteswap_ushort(toswap[1]);
  toswap[2] = _byteswap_ushort(toswap[2]);
  toswap[3] = _byteswap_ushort(toswap[3]);
#endif
}

__forceinline void SwapVectorBytes(uint32 toswap[4])
{
#if 0 // old __fastcall dependent code
  __asm
  {
    mov edx, [ecx]
    mov eax, [ecx + 4]
    bswap edx
    bswap eax
    mov [ecx],edx
    mov [ecx + 4],eax
    mov edx, [ecx + 8]
    mov eax, [ecx + 12]
    bswap edx
    bswap eax
    mov [ecx + 8],edx
    mov [ecx + 12],eax
  }
#else
  toswap[0] = _byteswap_ulong(toswap[0]);
  toswap[1] = _byteswap_ulong(toswap[1]);
  toswap[2] = _byteswap_ulong(toswap[2]);
  toswap[3] = _byteswap_ulong(toswap[3]);
#endif
}

#else

#define SwapWordBytes(x) 
#define SwapScalarBytes(x) 
#define SwapShortVectorBytes(x) 
#define SwapVectorBytes(x) 

#define SwapBytes(x) (x) 

#endif

#endif
