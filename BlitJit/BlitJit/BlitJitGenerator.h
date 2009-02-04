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

// [Guard]
#ifndef _BLITJITGENERATOR_H
#define _BLITJITGENERATOR_H

// [Dependencies]
#include "BlitJitConfig.h"

#include <AsmJit/AsmJitX86.h>

namespace BlitJit {

//! @addtogroup BlitJit_Main
//! @{

typedef void (BLITJIT_CALL *BlitSpanFn)(
  void* dst, const void* src, SysUInt len);
typedef void (BLITJIT_CALL *BlitSpanMaskFn)(
  void* dst, const void* src, const void* mask, SysUInt len);

struct Generator
{
  Generator(AsmJit::X86& a);
  ~Generator();

  //! @brief Emit function prolog and fetch parameters.
  //!
  //! Parameters:
  //! - dst = edi
  //! - src = esi
  //! - len = ecx
  virtual void entry();

  //! @brief Emit function epilog and restore preserved registers
  virtual void leave();

  //! @brief Generate span level blit function.
  virtual void blitSpan(UInt32 dId, UInt32 sId, UInt32 oId);

  //! @brief Assembler stream.
  AsmJit::X86& a;

private:
  // disable copy
  inline Generator(const Generator& other);
  inline Generator& operator=(const Generator& other);
};

//! @}

} // BlitJit namespace

// [Guard]
#endif // _BLITJITGENERATOR_H
