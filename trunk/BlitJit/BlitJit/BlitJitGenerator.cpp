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

#include "BlitJitApi.h"
#include "BlitJitGenerator.h"

#include <AsmJit/AsmJitAssembler.h>
#include <AsmJit/AsmJitCpuInfo.h>

using namespace AsmJit;

namespace BlitJit {

// ============================================================================
// [Generator - Construction / Destruction]
// ============================================================================

Generator::Generator(Assembler& a) : 
  a(a),
  features(cpuInfo()->features)
{
}

Generator::~Generator()
{
}

// ============================================================================
// [Generator - fillSpan]
// ============================================================================

void Generator::fillSpan(const PixelFormat& dst, const PixelFormat& src, const Operation& op)
{
  // Function entry
  fillSpanEntry();

  // If source pixel format not contains alpha channel, add it
  if (!src.isAlpha())
  {
    a.or_(eax, dst.aMask32());
  }

  /*
  I don't know if this is useable
  if (op.dstAlphaUsed() && !op.srcAlphaUsed())
  {
    a.or_(eax, dst.aMask32());
  }
  */

  // L_Begin
  a.bind(&L_Begin);

  // We can optimize simple fills in different way that compositing operations.
  // Simple fill should be unrolled loop for 32 bytes or more at a time.
  if (dst.depth() == 32 && op.id() == Operation::CombineCopy)
  {
    fillSpanMemSet32();
  }
  else
  {
    // Align
    Label L_Align;

    // a.mov(
    a.bind(&L_Align);

    if (op.dstPixelUsed())
    {
      switch (dst.id())
      {
        case PixelFormat::ARGB32:
        case PixelFormat::PRGB32:
        case PixelFormat::XRGB32:
          a.mov(edx, dword_ptr(edi));
          break;
      }
    }

    // Composite
    switch (op.id())
    {
      case Operation::CombineCopy:
        break;
    }

    // Store
    switch (dst.id())
    {
      case PixelFormat::ARGB32:
      case PixelFormat::PRGB32:
      case PixelFormat::XRGB32:
        a.mov(dword_ptr(edi), eax);
        break;
    }

    // End
    a.add(edi, dst.depth() / 8);
    a.dec(ecx);
    a.j(C_NOT_ZERO, &L_Begin);
  }

  // Function leave
  fillSpanLeave();
}

void Generator::fillSpanEntry()
{
  a.push(ebp);
  a.mov(ebp, esp);

  a.push(ebx);
  a.push(edi);

  const SysInt arg_offset = 8;
  a.mov(edi, dword_ptr(ebp, arg_offset + 0)); // dst
  a.mov(eax, dword_ptr(ebp, arg_offset + 4)); // src
  a.mov(ecx, dword_ptr(ebp, arg_offset + 8)); // len
}

void Generator::fillSpanLeave()
{
  a.bind(&L_Leave);

  a.pop(edi);
  a.pop(ebx);

  a.mov(esp, ebp);
  a.pop(ebp);
  a.ret(12);
}

void Generator::fillSpanMemSet32()
{
  Label L1;
  Label L2;

  a.cmp(ecx, 8);
  a.j(C_LESS, &L2);

  a.mov(edx, ecx);
  a.shr(ecx, 3);
  a.and_(edx, 7);

  // Unrolled loop, we are using align 4 here, because this path will be 
  // generated only for older CPUs.
  a.align(4);
  a.bind(&L1);

  a.mov(dword_ptr(edi,  0), eax);
  a.mov(dword_ptr(edi,  4), eax);
  a.mov(dword_ptr(edi,  8), eax);
  a.mov(dword_ptr(edi, 12), eax);
  a.mov(dword_ptr(edi, 16), eax);
  a.mov(dword_ptr(edi, 20), eax);
  a.mov(dword_ptr(edi, 24), eax);
  a.mov(dword_ptr(edi, 28), eax);

  a.add(edi, 32);
  a.dec(ecx);
  a.j(C_NOT_ZERO, &L1);

  a.mov(ecx, edx);
  a.j(C_ZERO, &L_Leave);

  // Tail
  a.align(4);
  a.bind(&L2);

  a.mov(dword_ptr(edi,  0), eax);

  a.add(edi, 4);
  a.dec(ecx);
  a.j(C_NOT_ZERO, &L2);
}

// ============================================================================
// [Generator - blitSpan]
// ============================================================================

void Generator::blitSpan(const PixelFormat& dst, const PixelFormat& src, const Operation& op)
{
  // Function entry
  blitSpanEntry();

  // Loop
  a.bind(&L_Begin);

  // Fetch
  if (op.srcPixelUsed())
  {
    switch (op.id())
    {
      case PixelFormat::ARGB32:
      case PixelFormat::PRGB32:
      case PixelFormat::XRGB32:
        a.mov(eax, dword_ptr(esi));
        break;
    }
  }

  if (op.dstPixelUsed())
  {
    switch (dst.id())
    {
      case PixelFormat::ARGB32:
      case PixelFormat::PRGB32:
      case PixelFormat::XRGB32:
        a.mov(edx, dword_ptr(edi));
        break;
    }
  }

  // Composite
  switch (op.id())
  {
    case Operation::CombineCopy:
      break;
  }

  if (op.dstAlphaUsed() && !op.srcAlphaUsed())
  {
    a.or_(eax, dst.aMask32());
  }

  // Store
  switch (dst.id())
  {
    case PixelFormat::ARGB32:
    case PixelFormat::PRGB32:
    case PixelFormat::XRGB32:
      a.mov(dword_ptr(edi), eax);
      break;
  }

  // End
  a.add(esi, src.depth() / 8);
  a.add(edi, dst.depth() / 8);
  a.dec(ecx);
  a.j(C_NOT_ZERO, &L_Begin);

  // Function leave
  blitSpanLeave();
}

void Generator::blitSpanEntry()
{
  a.push(ebp);
  a.mov(ebp, esp);

  a.push(ebx);
  a.push(edi);
  a.push(esi);

  const SysInt arg_offset = 8;
  a.mov(edi, dword_ptr(ebp, arg_offset + 0)); // dst
  a.mov(esi, dword_ptr(ebp, arg_offset + 4)); // src
  a.mov(ecx, dword_ptr(ebp, arg_offset + 8)); // len
}

void Generator::blitSpanLeave()
{
  a.bind(&L_Leave);

  a.pop(esi);
  a.pop(edi);
  a.pop(ebx);

  a.mov(esp, ebp);
  a.pop(ebp);
  a.ret(12);
}

} // BlitJit namespace
