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

// [Dependencies]
#include "BlitJitBuild.h"
#include "BlitJitApi.h"
#include "BlitJitCodeManager.h"
#include "BlitJitDefs.h"
#include "BlitJitGenerator.h"
#include "BlitJitLock.h"
#include "BlitJitMemoryManager.h"

namespace BlitJit {

static inline SysUInt calcPfPfOp(SysUInt dId, SysUInt sId, SysUInt oId)
{
  return (dId * Operation::Count * PixelFormat::Count +
          sId * Operation::Count +
          oId);
}

CodeManager::CodeManager()
{
  memset(_fillSpan, 0, sizeof(_fillSpan));
  memset(_blitSpan, 0, sizeof(_blitSpan));
  memset(_fillRect, 0, sizeof(_fillRect));
  memset(_blitRect, 0, sizeof(_blitRect));
}

CodeManager::~CodeManager()
{
}

FillSpanFn CodeManager::getFillSpan(UInt32 dId, UInt32 sId, UInt32 oId)
{
  SysUInt pos = calcPfPfOp(dId, sId, oId);
  FillSpanFn fn;
  
  if ((fn = _fillSpan[pos]) != NULL)
  {
    return fn;
  }
  else
  {
    AutoLock _locked(_lock);
    // If function was generated within non locked time
    if ((fn = _fillSpan[pos]) != NULL) return fn;

    AsmJit::Compiler c;
    Generator gen(&c);

    gen.genFillSpan(
      Api::pixelFormats[dId],
      Api::pixelFormats[sId], 
      Api::operations[oId]);
    fn = AsmJit::function_cast<FillSpanFn>(_memmgr.submit(c));

    _fillSpan[pos] = fn;
    return fn;
  }
}

FillRectFn CodeManager::getFillRect(UInt32 dId, UInt32 sId, UInt32 oId)
{
  SysUInt pos = calcPfPfOp(dId, sId, oId);
  FillRectFn fn;
  
  if ((fn = _fillRect[pos]) != NULL)
  {
    return fn;
  }
  else
  {
    AutoLock _locked(_lock);
    // If function was generated within non locked time
    if ((fn = _fillRect[pos]) != NULL) return fn;

    AsmJit::Compiler c;
    Generator gen(&c);

    gen.genFillRect(
      Api::pixelFormats[dId],
      Api::pixelFormats[sId], 
      Api::operations[oId]);
    fn = AsmJit::function_cast<FillRectFn>(_memmgr.submit(c));

    _fillRect[pos] = fn;
    return fn;
  }
}

BlitSpanFn CodeManager::getBlitSpan(UInt32 dId, UInt32 sId, UInt32 oId)
{
  SysUInt pos = calcPfPfOp(dId, sId, oId);
  BlitSpanFn fn;
  
  if ((fn = _blitSpan[pos]) != NULL)
  {
    return fn;
  }
  else
  {
    AutoLock locked(_lock);
    // If function was generated within non locked time
    if ((fn = _blitSpan[pos]) != NULL) return fn;

    AsmJit::Compiler c;
    Generator gen(&c);

    gen.genBlitSpan(
      Api::pixelFormats[dId],
      Api::pixelFormats[sId], 
      Api::operations[oId]);
    fn = AsmJit::function_cast<BlitSpanFn>(_memmgr.submit(c));

    _blitSpan[pos] = fn;
    return fn;
  }
}

BlitRectFn CodeManager::getBlitRect(UInt32 dId, UInt32 sId, UInt32 oId)
{
  SysUInt pos = calcPfPfOp(dId, sId, oId);
  BlitRectFn fn;
  
  if ((fn = _blitRect[pos]) != NULL)
  {
    return fn;
  }
  else
  {
    AutoLock locked(_lock);
    // If function was generated within non locked time
    if ((fn = _blitRect[pos]) != NULL) return fn;

    AsmJit::Compiler c;
    Generator gen(&c);

    gen.genBlitRect(
      Api::pixelFormats[dId],
      Api::pixelFormats[sId], 
      Api::operations[oId]);
    fn = AsmJit::function_cast<BlitRectFn>(_memmgr.submit(c));

    _blitRect[pos] = fn;
    return fn;
  }
}

} // BlitJit namespace
