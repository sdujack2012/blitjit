// BlitJit - Just In Time Image Blitting Library.

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

// [Dependencies]
#include "BlitJitConfig.h"
#include "BlitJitLock.h"
#include "BlitJitMemoryManager.h"

#include <AsmJit/AsmJitConfig.h>
#include <AsmJit/AsmJitVM.h>

#include <string.h>

namespace BlitJit {

static SysUInt getDefaultVSize()
{
  SysUInt size = AsmJit::VM::pageSize();
  while (size < 32768) size <<= 1;
  return size;
}

MemoryManager::MemoryManager()
{
  _alignment = 32;
  _virtualSize = getDefaultVSize();
  _chunks = NULL;
}

MemoryManager::~MemoryManager()
{
  Chunk* cur = _chunks;
  while (cur)
  {
    Chunk* prev = cur->prev;
    AsmJit::VM::free(cur->base, cur->capacity);
    BLITJIT_FREE(cur);
    cur = prev;
  }
}

void* MemoryManager::submit(const void* code, SysUInt size)
{
  SysUInt over = size % _alignment;
  if (over) over = _alignment - over;
  SysUInt alignedSize = size + over;

  AutoLock locked(_lock);
  Chunk* cur = _chunks;

  // Try to find space in allocated chunks
  while (cur && alignedSize > cur->available()) cur = cur->prev;

  // Or allocate new chunk
  if (!cur)
  {
    cur = (Chunk*)BLITJIT_MALLOC(sizeof(Chunk));
    if (cur == NULL) return NULL;

    cur->base = (UInt8*)AsmJit::VM::alloc(_virtualSize, &cur->capacity, true);
    if (cur->base == NULL) 
    {
      BLITJIT_FREE(cur);
      return NULL;
    }

    cur->used = 0;
    cur->prev = _chunks;
    _chunks = cur;
  }

  // It's possible for jit generated function to be larger than our
  // default virtual memory size (32kB) ?
  if (alignedSize > cur->available()) return NULL;

  // Finally, copy function code to our space we reserved for.
  UInt8* p = cur->base + cur->used;
  cur->used += alignedSize;

  // Code can be null to only reserve space for code.
  if (code) memcpy(p, code, size);
  memset(p + size, 0xCC, over); // int3
  return (void*)p;
}

void* MemoryManager::submit(const AsmJit::Assembler& a)
{
  SysUInt size = static_cast<SysUInt>(a.codeSize());
  void* m = submit(NULL, size);

  if (m) a.relocCode(m);
  return m;
}

} // BlitJit namespace
