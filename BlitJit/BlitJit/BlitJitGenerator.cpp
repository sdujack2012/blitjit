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

#include <AsmJit/AsmJitX86.h>
using namespace AsmJit;

namespace BlitJit {

Generator::Generator(X86& a) : a(a)
{
}

Generator::~Generator()
{
}

void Generator::blitSpan(UInt32 dId, UInt32 sId, UInt32 oId)
{
  const PixelFormat& d = Api::pixelFormats[dId];
  const PixelFormat& s = Api::pixelFormats[sId];
  const Operation& o = Api::operations[oId];

  // Function entry
  entry();

  // Loop
  Label L_Begin;
  a.bind(&L_Begin);

  // Fetch
  if (o.srcPixelUsed)
  {
    switch (o.id)
    {
      case PixelFormat::ARGB32:
      case PixelFormat::PRGB32:
      case PixelFormat::XRGB32:
        a.mov(eax, dword_ptr(esi));
        break;
    }
  }

  if (o.dstPixelUsed)
  {
    switch (d.id)
    {
      case PixelFormat::ARGB32:
      case PixelFormat::PRGB32:
      case PixelFormat::XRGB32:
        a.mov(edx, dword_ptr(edi));
        break;
    }
  }

  // Composite
  switch (o.id)
  {
    case Operation::CombineCopy:
      break;
  }

  if (o.dstAlphaUsed && !o.srcAlphaUsed)
  {
    a.or_(eax, d.aMask);
  }

  // Store
  switch (d.id)
  {
    case PixelFormat::ARGB32:
    case PixelFormat::PRGB32:
    case PixelFormat::XRGB32:
      a.mov(dword_ptr(edi), eax);
      break;
  }

  // End
  a.add(esi, s.depth / 8);
  a.add(edi, d.depth / 8);
  a.dec(ecx);
  a.j(C_NOT_ZERO, &L_Begin);

  // Function leave
  leave();
}

void Generator::entry()
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

void Generator::leave()
{
  a.pop(esi);
  a.pop(edi);
  a.pop(ebx);

  a.mov(esp, ebp);
  a.pop(ebp);
  a.ret(12);
}

} // BlitJit namespace
