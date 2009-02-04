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
#ifndef _BLITJITDEFS_H
#define _BLITJITDEFS_H

// [Dependencies]
#include "BlitJitConfig.h"
#include "BlitJitLock.h"

namespace BlitJit {

//! @addtogroup BlitJit_Main
//! @{

//! @brief Pixel format.
struct PixelFormat
{
  char name[32];
  UInt32 id;

  UInt32 depth;

  UInt32 rMask;
  UInt32 gMask;
  UInt32 bMask;
  UInt32 aMask;

  UInt32 rShift;
  UInt32 gShift;
  UInt32 bShift;
  UInt32 aShift;

  UInt8 rBytePos;
  UInt8 gBytePos;
  UInt8 bBytePos;
  UInt8 aBytePos;

  UInt32 isPremultiplied : 1;
  UInt32 isIndexed : 1;

  enum Id
  {
    ARGB32 = 0,
    PRGB32 = 1,
    XRGB32 = 2,

    Count
  };
};

//! @brief Operation.
struct Operation
{
  char name[32];
  UInt32 id;

  UInt32 srcPixelUsed : 1;
  UInt32 dstPixelUsed : 1;

  UInt32 srcAlphaUsed : 1;
  UInt32 dstAlphaUsed : 1;

  enum Id
  {
    // TODO: Unify this

    // [Destination WITHOUT Alpha Channel]

    //! @brief Copy source to dest ignoring it's alpha value
    CombineCopy = 0,
    //! @brief Blend source to dest using source alpha value.
    CombineBlend = 1,

    // [Destination WITH Alpha Channel]

    //! @brief Clear alpha value.
    CompositeClear,
    CompositeSrc,
    CompositeDest,
    CompositeOver,
    CompositeOverReverse,
    CompositeIn,
    CompositeInReverse,
    CompositeOut,
    CompositeOutReverse,
    CompositeAtop,
    CompositeAtopReverse,
    CompositeXor,
    CompositeAdd,
    CompositeSaturate,

    Count
  };
};


//! @}

} // BlitJit namespace

// [Guard]
#endif // _BLITJITDEFS_H
