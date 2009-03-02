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
#ifndef _BLITJITDEFS_H
#define _BLITJITDEFS_H

// [Dependencies]
#include <AsmJit/AsmJitConfig.h>
#include <AsmJit/AsmJitCompiler.h>

#include "BlitJitConfig.h"
#include "BlitJitLock.h"

namespace BlitJit {

//! @addtogroup BlitJit_Main
//! @{

// [BlitJit - Calling Convention]

//! @def BLITJIT_CALL
//! @brief Calling convention of function generated by BlitJit

#if defined(BLITJIT_X86)
# define BLITJIT_CALL ASMJIT_STDCALL
enum { CConv = AsmJit::CALL_CONV_STDCALL };
#else
# define BLITJIT_CALL
enum { CConv = AsmJit::CALL_CONV_DEFAULT };
#endif


//! @brief Fill span function prototype.
typedef void (BLITJIT_CALL *FillSpanFn)(
  void* dst, const UInt32 src, SysUInt len);

//! @brief Blit span function prototype.
typedef void (BLITJIT_CALL *BlitSpanFn)(
  void* dst, const void* src, SysUInt len);

//! @brief Blit span with extra mask function prototype.
typedef void (BLITJIT_CALL *BlitSpanMaskFn)(
  void* dst, const void* src, const void* mask, SysUInt len);


//! @brief Fill rect function prototype.
typedef void (BLITJIT_CALL *FillRectFn)(
  void* dst, const void* src, SysUInt len);

//! @brief Blit rect function prototype.
typedef void (BLITJIT_CALL *BlitRectFn)(
  void* dst, const void* src,
  SysInt dstStride, SysInt srcStride,
  SysUInt width, SysUInt height);

//! @brief Blit rect with extra mask function prototype.
typedef void (BLITJIT_CALL *BlitRectMaskFn)(
  void* dst, const void* src, const void* mask,
  SysInt dstStride, SysInt srcStride, SysInt maskStride,
  SysUInt width, SysUInt height);


//! @brief Pixel format.
struct PixelFormat
{
  inline const char* name() const { return _name; }
  inline UInt32 id() const { return _id; }

  inline UInt32 depth() const { return _depth; }

  inline UInt32 rMask16() const { return (((UInt16)1U << _rSize) - 1) << _rShift; }
  inline UInt32 gMask16() const { return (((UInt16)1U << _gSize) - 1) << _gShift; }
  inline UInt32 bMask16() const { return (((UInt16)1U << _bSize) - 1) << _bShift; }
  inline UInt32 aMask16() const { return (((UInt16)1U << _aSize) - 1) << _aShift; }

  inline UInt32 rMask32() const { return ((1U << _rSize) - 1) << _rShift; }
  inline UInt32 gMask32() const { return ((1U << _gSize) - 1) << _gShift; }
  inline UInt32 bMask32() const { return ((1U << _bSize) - 1) << _bShift; }
  inline UInt32 aMask32() const { return ((1U << _aSize) - 1) << _aShift; }

  inline UInt32 rMask64() const { return (((UInt64)1U << _rSize) - 1) << _rShift; }
  inline UInt32 gMask64() const { return (((UInt64)1U << _gSize) - 1) << _gShift; }
  inline UInt32 bMask64() const { return (((UInt64)1U << _bSize) - 1) << _bShift; }
  inline UInt32 aMask64() const { return (((UInt64)1U << _aSize) - 1) << _aShift; }

  inline UInt32 rShift() const { return _rShift; }
  inline UInt32 gShift() const { return _gShift; }
  inline UInt32 bShift() const { return _bShift; }
  inline UInt32 aShift() const { return _aShift; }

  inline UInt32 isPremultiplied() const { _isPremultiplied; }
  inline UInt32 isFloat() const { return _isFloat; }

  inline bool isArgb() const { return _rSize != 0 && _gSize != 0 && _bSize != 0 && _aSize != 0; }
  inline bool isAlpha() const { return _aSize != 0; }
  inline bool isGrey() const { return (_rShift == _gShift) & (_gShift == _bShift); }
  inline bool isRgb() const { return _rSize != 0 && _gSize != 0 && _bSize != 0; }

  //! @brief Pixel format IDs.
  enum Id
  {
    //! @brief 32 bit RGB format with alpha, colors are not premultiplied by alpha.
    ARGB32,
    //! @brief 32 bit RGB format with alpha, colors are premultiplied by alpha.
    PRGB32,
    //! @brief 32 bit RGB format without alpha. One byte is unused.
    XRGB32,

    //! @brief 24 bit RGB format in Big Endian Order.
    RGB24,
    //! @brief 24 bit RGB format in Little Endian Order.
    BGR24,

    //! @brief Count of formats.
    Count
  };

  // variables, not private, because this is structure usually read-only.

  char _name[32];
  UInt32 _id;

  UInt32 _depth;

  UInt32 _rSize;
  UInt32 _gSize;
  UInt32 _bSize;
  UInt32 _aSize;

  UInt32 _rShift;
  UInt32 _gShift;
  UInt32 _bShift;
  UInt32 _aShift;

  UInt32 _isPremultiplied : 1;
  UInt32 _isFloat : 1;
};


//! @brief Operation.
struct Operation
{
  inline const char* name() const { return _name; }
  inline UInt32 id() const { return _id; }

  inline UInt8 srcPixelUsed() const { return _srcPixelUsed; }
  inline UInt8 dstPixelUsed() const { return _dstPixelUsed; }
  inline UInt8 srcAlphaUsed() const { return _srcAlphaUsed; }
  inline UInt8 dstAlphaUsed() const { return _dstAlphaUsed; }

  enum Id
  {
    //! @brief Source to dest (dest will be altered).
    CompositeSrc,

    //! @brief Dest to source (source will be altered).
    CompositeDest,

    //! @brief Source over dest.
    CompositeOver,

    //! @brief Dest over source.
    CompositeOverReverse,

    //! @brief Source in dest.
    CompositeIn,

    //! @brief Dest in source.
    CompositeInReverse,

    //! @brief Source out dest.
    CompositeOut,

    //! @brief Dest out source.
    CompositeOutReverse,

    //! @brief Source atop dest.
    CompositeAtop,

    //! @brief Dest atop source.
    CompositeAtopReverse,

    //! @brief Source xor dest.
    CompositeXor,

    //! @brief Clear to fully transparent or black (if image not contains alpha 
    //! channel).
    CompositeClear,

    CompositeAdd,

    CompositeSubtract,

    CompositeMultiply,

    CompositeScreen,

    CompositeDarken,

    CompositeLighten,

    CompositeDifference,

    CompositeExclusion,

    CompositeInvert,

    CompositeInvertRgb,

    CompositeSaturate,

    //! @brief Count of operators.
    Count
  };

  // variables, not private, because this structure is usually read-only.

  char _name[32];
  UInt32 _id;

  UInt8 _srcPixelUsed;
  UInt8 _dstPixelUsed;

  UInt8 _srcAlphaUsed;
  UInt8 _dstAlphaUsed;
};

//! @}

} // BlitJit namespace

// [Guard]
#endif // _BLITJITDEFS_H
