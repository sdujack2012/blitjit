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

typedef void (BLITJIT_CALL *FillSpanFn)(
  void* dst, const UInt32 src, SysUInt len);
typedef void (BLITJIT_CALL *BlitSpanFn)(
  void* dst, const void* src, SysUInt len);
typedef void (BLITJIT_CALL *BlitSpanMaskFn)(
  void* dst, const void* src, const void* mask, SysUInt len);

struct Generator
{
  Generator(AsmJit::X86& a);
  virtual ~Generator();

  // [FillSpan]

  //! @brief Generate fill span function.
  virtual void fillSpan(const PixelFormat& dst, const PixelFormat& src, const Operation& op);

  //! @brief Emit function prolog and fetch parameters.
  //!
  //! Saves registers:
  //! - ebx, edi
  //!
  //! Parameters:
  //! - [ptr dst] = edi
  //! - src = eax
  //! - len = ecx
  virtual void fillSpanEntry();

  //! @brief Emit function epilog and restore preserved registers
  //!
  //! Loads registers:
  //! - edi, ebx
  //!
  virtual void fillSpanLeave();

  virtual void fillSpanMemSet32();

  // [BlitSpan]

  //! @brief Generate blit span function.
  virtual void blitSpan(const PixelFormat& dst, const PixelFormat& src, const Operation& op);

  //! @brief Emit function prolog and fetch parameters.
  //!
  //! Saves registers:
  //! - ebx, edi, esi
  //!
  //! Parameters:
  //! - [ptr dst] = edi
  //! - [ptr src] = esi
  //! - len = ecx
  virtual void blitSpanEntry();

  //! @brief Emit function epilog and restore preserved registers
  //!
  //! Loads registers:
  //! - esi, edi, ebx
  //!
  virtual void blitSpanLeave();

  //! @brief Assembler stream.
  AsmJit::X86& a;
  //! @brief Begin of function after prolog.
  AsmJit::Label L_Begin;
  //! @brief Label before epilog.
  AsmJit::Label L_Leave;

private:
  // disable copy
  inline Generator(const Generator& other);
  inline Generator& operator=(const Generator& other);
};

//! @}

} // BlitJit namespace

// [Guard]
#endif // _BLITJITGENERATOR_H
