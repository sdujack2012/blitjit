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

// [Guard]
#ifndef _BLITJITMEMORYMANAGER_H
#define _BLITJITMEMORYMANAGER_H

// [Dependencies]
#include <AsmJit/AsmJitBuild.h>
#include <AsmJit/AsmJitAssembler.h>
#include <AsmJit/AsmJitCompiler.h>

#include "BlitJitBuild.h"
#include "BlitJitLock.h"

namespace BlitJit {

//! @addtogroup BlitJit_Platform
//! @{

//! @brief Memory manager that stores all jit generated functions.
//!
//! Functions stored by this manager will be never freed. To allocate 
//! virtual memory with execution enabled is used @c AsmJit::VM class.
struct BLITJIT_API MemoryManager
{
  MemoryManager();
  ~MemoryManager();

  //! @brief Return alignment to use for functions.
  inline SysUInt alignment() const { return _alignment; }
  //! @brief Return default size of each memory block.
  inline SysUInt virtualSize() const { return _virtualSize; }
  //! @brief Return total size of used memory by all functions.
  inline SysUInt used() const { return _used; }

  //! @brief Adds binary code and returns pointer where it was placed.
  //!
  //! Returned pointer will be never changed.
  //!
  //! @note This function can return NULL if memory couldn't be allocated.
  //!
  //! @note @a code can be NULL and after call of this function it can be
  //! manualy copied to returned address.
  void* submit(const void* code, SysUInt size);

  //! @overload
  void* submit(AsmJit::Assembler& a);
  void* submit(AsmJit::Compiler& c);

  //! @brief Memory Manager Chunk.
  struct Chunk
  {
    //! @brief Base pointer
    UInt8* base;

    //! @brief Count of bytes used.
    SysUInt used;
    //! @brief Count of bytes allocated.
    SysUInt capacity;

    //! @brief Pointer to previous chunk or NULL if there is no one.
    //!
    //! @note Previous chunks are allocated
    Chunk* prev;

    //! @brief Return available space.
    inline SysUInt available() const { return capacity - used; }
  };

private:
  //! @brief Alignment of each function (defaults is 32 bytes).
  SysUInt _alignment;
  //! @brief Default size of virtual memory to allocate.
  SysUInt _virtualSize;
  //! @brief Total used memory by functions.
  SysUInt _used;
  //! @brief Chunks.
  Chunk* _chunks;
  //! @brief Lock.
  Lock _lock;

  // disable copy
  inline MemoryManager(const MemoryManager& other);
  inline MemoryManager& operator=(const MemoryManager& other);
};

//! @}

} // BlitJit namespace

// [Guard]
#endif // _BLITJITMEMORYMANAGER_H
