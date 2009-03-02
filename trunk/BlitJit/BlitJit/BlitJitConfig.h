// BlitJit - Just In Time Image Blitting Library for C++ Language.

// Copyright (c) 2008-2009, Petr Kobalicek <kobalicek.petr@gmail.com>
//
// Permission is hereby granted, free of charge, to any person
// obtaining a copy of this software and associated documentation
// files (the "Software"), to deal in the Software without
// restriction, including without limitation the rights to use,
// copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following
// conditions:
// 
// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
// OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
// HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
// WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
// OTHER DEALINGS IN THE SOFTWARE.

// This file is designed to be changeable. Platform specific changes
// should be applied to this file and this guarantes and never versions
// of BlitJit library will never overwrite generated config files.
//
// So modify this will by your build system or hand.

// [Guard]
#ifndef _BLITJITCONFIG_H
#define _BLITJITCONFIG_H

// [Include]
//
// Here should be optional include files that's needed fo successfuly
// use macros defined here. Remember, BlitJit uses only BlitJit namespace
// and all macros are used within it. So for example crash handler is
// not called as BlitJit::crash(0) in BLITJIT_ILLEGAL() macro, but simply
// as crash(0).
#include <stdlib.h>

// depends to AsmJit
#include <AsmJit/AsmJitConfig.h>

// [BlitJit - OS]
#define BLITJIT_WINDOWS ASMJIT_WINDOWS
#define BLITJIT_POSIX ASMJIT_POSIX

#if !defined(BLITJIT_OS)
# define BLITJIT_OS ASMJIT_OS
#endif

#if !defined(BLITJIT_OS) || BLITJIT_OS < 1
#error "BlitJit - Define BLITJIT_OS macro to your operating system"
#endif // BLITJIT_OS

// [BlitJit - API]
#define BLITJIT_API
#define BLITJIT_VAR extern

// [BlitJit - Memory]
#if !defined(BLITJIT_MALLOC)
# define BLITJIT_MALLOC ::malloc
#endif // BLITJIT_MALLOC

#if !defined(BLITJIT_REALLOC)
# define BLITJIT_REALLOC ::realloc
#endif // BLITJIT_REALLOC

#if !defined(BLITJIT_FREE)
# define BLITJIT_FREE ::free
#endif // BLITJIT_FREE

// [BlitJit - Crash handler]
namespace BlitJit
{
  static void crash(int* ptr = 0) { *ptr = 0; }
}

// [BlitJit - Architecture]
// define it only if it's not defined. In some systems we can
// use -D command in compiler to bypass this autodetection.
#if !defined(BLITJIT_X86) && !defined(BLITJIT_X64)
# if defined(ASMJIT_X64)
#  define BLITJIT_X64
# else
#  define BLITJIT_X86
# endif
#endif

// [BlitJit - Types]
namespace BlitJit
{
  // Standard types
  using AsmJit::Int8;
  using AsmJit::UInt8;
  using AsmJit::Int16;
  using AsmJit::UInt16;
  using AsmJit::Int32;
  using AsmJit::UInt32;
  using AsmJit::Int64;
  using AsmJit::UInt64;

  using AsmJit::SysInt;
  using AsmJit::SysUInt;

  // Types used in computer graphics
  typedef UInt8 Data8;
  typedef UInt16 Data16;
  typedef UInt32 Data32;
  typedef UInt64 Data64;
}

#if defined(_MSC_VER)
# define BLITJIT_INT64_C(num) num##i64
# define BLITJIT_UINT64_C(num) num##ui64
#else
# define BLITJIT_INT64_C(num) num##LL
# define BLITJIT_UINT64_C(num) num##ULL
#endif

// [BlitJit - Debug]
#if defined(DEBUG) || 1
# if !defined(BLITJIT_CRASH)
#  define BLITJIT_CRASH() crash()
# endif
# if !defined(BLITJIT_ASSERT)
#  define BLITJIT_ASSERT(exp) do { if (!(exp)) BLITJIT_CRASH(); } while(0)
# endif
#else
# if !defined(BLITJIT_CRASH)
#  define BLITJIT_CRASH() do {} while(0)
# endif
# if !defined(BLITJIT_ASSERT)
#  define BLITJIT_ASSERT(exp) do {} while(0)
# endif
#endif // DEBUG

// [Guard]
#endif // _BLITJITCONFIG_H
