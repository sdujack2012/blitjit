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
#ifndef _BLITJITCODEMANAGER_H
#define _BLITJITCODEMANAGER_H

// [Dependencies]
#include <AsmJit/AsmJitCompiler.h>
#include <AsmJit/AsmJitLogger.h>

#include "BlitJitBuild.h"
#include "BlitJitDefs.h"
#include "BlitJitGenerator.h"
#include "BlitJitLock.h"
#include "BlitJitMemoryManager.h"

namespace BlitJit {

//! @addtogroup BlitJit_Main
//! @{

struct CodeManager
{
  CodeManager();
  ~CodeManager();

  void configureCompiler(AsmJit::Compiler* c);

  FillSpanFn getFillSpan(UInt32 dId, UInt32 sId, UInt32 oId);
  FillRectFn getFillRect(UInt32 dId, UInt32 sId, UInt32 oId);

  BlitSpanFn getBlitSpan(UInt32 dId, UInt32 sId, UInt32 oId);
  BlitRectFn getBlitRect(UInt32 dId, UInt32 sId, UInt32 oId);

  inline MemoryManager& memmgr() { return _memmgr; }

  // Logger
  AsmJit::FileLogger _logger;

  // Functions
  enum
  {
    FillSpanFnCount = PixelFormat::Count * PixelFormat::Count * Operation::Count,
    FillRectFnCount = PixelFormat::Count * PixelFormat::Count * Operation::Count,

    BlitSpanFnCount = PixelFormat::Count * PixelFormat::Count * Operation::Count,
    BlitRectFnCount = PixelFormat::Count * PixelFormat::Count * Operation::Count 
  };

  // Cache
  FillSpanFn _fillSpan[FillSpanFnCount];
  FillRectFn _fillRect[FillSpanFnCount];

  BlitSpanFn _blitSpan[BlitSpanFnCount];
  BlitRectFn _blitRect[BlitRectFnCount];

  // Memory manager
  MemoryManager _memmgr;

  // Lock
  Lock _lock;
};

//! @}

} // BlitJit namespace

// [Guard]
#endif // _BLITJITCODEMANAGER_H
