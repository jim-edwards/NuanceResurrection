/*
        Compatibility <intrin.h> header for GCC -- GCC equivalents of intrinsic
        Microsoft Visual C++ functions. Originally developed for the ReactOS
        (<http://www.reactos.org/>) and TinyKrnl (<http://www.tinykrnl.org/>)
        projects.

        Copyright (c) 2006 KJK::Hyperion <hackbunny@reactos.com>

        Permission is hereby granted, free of charge, to any person obtaining a
        copy of this software and associated documentation files (the
   "Software"), to deal in the Software without restriction, including without
   limitation the rights to use, copy, modify, merge, publish, distribute,
   sublicense, and/or sell copies of the Software, and to permit persons to whom
   the Software is furnished to do so, subject to the following conditions:

        The above copyright notice and this permission notice shall be included
   in all copies or substantial portions of the Software.

        THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
   OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
        FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
   THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
        LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
        FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
        DEALINGS IN THE SOFTWARE.
*/

#ifndef KJK_INTRIN_H_
#define KJK_INTRIN_H_

#ifndef __GNUC__
#error Unsupported compiler
#endif

/*
        FIXME: review all "memory" clobbers, add/remove to match Visual C++
        behavior: some "obvious" memory barriers are not present in the Visual
   C++ implementation - e.g. __stosX; on the other hand, some memory barriers
   that *are* present could have been missed
*/

/*
        NOTE: this is a *compatibility* header. Some functions may look wrong at
        first, but they're only "as wrong" as they would be on Visual C++. Our
        priority is compatibility

        NOTE: unlike most people who write inline asm for GCC, I didn't pull the
        constraints and the uses of __volatile__ out of my... hat. Do not touch
        them. I hate cargo cult programming

        NOTE: be very careful with declaring "memory" clobbers. Some "obvious"
        barriers aren't there in Visual C++ (e.g. __stosX)

        NOTE: review all intrinsics with a return value, add/remove __volatile__
        where necessary. If an intrinsic whose value is ignored generates a
   no-op under Visual C++, __volatile__ must be omitted; if it always generates
   code (for example, if it has side effects), __volatile__ must be specified.
   GCC will only optimize out non-volatile asm blocks with outputs, so
   input-only blocks are safe. Oddities such as the non-volatile 'rdmsr' are
   intentional and follow Visual C++ behavior

        NOTE: on GCC 4.1.0, please use the __sync_* built-ins for barriers and
        atomic operations. Test the version like this:

        #if (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__) >
   40100
                ...

        Pay attention to the type of barrier. Make it match with what Visual C++
        would use in the same case
*/

// HAS_BUILTIN is used to prevent the redefining of some buildin function in clang:
// e.g.:
// ./intrin_x86.h:837:69: error: definition of builtin function '__rdtsc'
// static __inline__ __attribute__((always_inline)) unsigned long long
// __rdtsc(void)
#ifdef __clang__
#define HAS_BUILTIN(x) __has_builtin(x)
#else
#define HAS_BUILTIN(x) 0
#endif

#if defined(__i386__)
#include "intrin_x86.h"
#elif defined(_PPC_)
#include "intrin_ppc.h"
#elif defined(_MIPS_)
#include "intrin_mips.h"
#elif defined(__x86_64__)
/* TODO: the x64 architecture shares most of the i386 intrinsics. It should be
 * easy to support */
#include "x86intrin.h"
#else
#error Unsupported architecture
#endif

/*** Miscellaneous ***/
/* BUGBUG: only good for use in macros. Cannot be taken the address of */
#define __noop(...) ((void)0)

/* TODO: __assume. GCC only supports the weaker __builtin_expect */

#endif

/* EOF */