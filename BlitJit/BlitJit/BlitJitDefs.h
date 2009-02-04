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
  inline const char* name() const { return _name; }
  inline UInt32 id() const { return _id; }

  inline UInt32 depth() const { return _depth; }

  inline UInt32 rMask32() const { return _rMask; }
  inline UInt32 gMask32() const { return _gMask; }
  inline UInt32 bMask32() const { return _bMask; }
  inline UInt32 aMask32() const { return _aMask; }

  inline UInt32 rShift32() const { return _rShift; }
  inline UInt32 gShift32() const { return _gShift; }
  inline UInt32 bShift32() const { return _bShift; }
  inline UInt32 aShift32() const { return _aShift; }

  inline UInt32 rBytePos() const { return _rBytePos; }
  inline UInt32 gBytePos() const { return _gBytePos; }
  inline UInt32 bBytePos() const { return _bBytePos; }
  inline UInt32 aBytePos() const { return _aBytePos; }

  inline bool isPremultiplied() const { return static_cast<bool>(_isPremultiplied); }

  inline bool isRgb() const
  {
    return _rMask != 0 && _gMask != 0 && _bMask != 0;
  }
  
  inline bool isGrey() const
  {
    return _rMask == _gMask && _gMask == _bMask;
  }

  inline bool isAlpha() const
  {
    return _aMask != 0;
  }

  //! @brief Pixel format IDs.
  enum Id
  {
    ARGB32,
    PRGB32,
    XRGB32,

    RGB24,
    BGR24,

    Count
  };

  // variables, not private, because this is structure usually read-only.

  char _name[32];
  UInt32 _id;

  UInt32 _depth;

  UInt32 _rMask;
  UInt32 _gMask;
  UInt32 _bMask;
  UInt32 _aMask;

  UInt32 _rShift;
  UInt32 _gShift;
  UInt32 _bShift;
  UInt32 _aShift;

  UInt32 _rBytePos;
  UInt32 _gBytePos;
  UInt32 _bBytePos;
  UInt32 _aBytePos;

  UInt32 _isPremultiplied : 1;
};

//! @brief Operation.
struct Operation
{
  inline const char* name() const { return _name; }
  inline UInt32 id() const { return _id; }

  inline bool srcPixelUsed() const { return _srcPixelUsed; }
  inline bool dstPixelUsed() const { return _dstPixelUsed; }
  inline bool srcAlphaUsed() const { return _srcAlphaUsed; }
  inline bool dstAlphaUsed() const { return _dstAlphaUsed; }

  enum Id
  {
    // TODO: Unify this

    // [Destination WITHOUT Alpha Channel]

    //! @brief Copy source to dest ignoring it's alpha value
    CombineCopy = 0,
    //! @brief Blend source to dest using source alpha value.
    CombineBlend = 1,

#if 0 // not used for now
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
#endif
    Count
  };

  // variables, not private, because this is structure usually read-only.

  char _name[32];
  UInt32 _id;

  UInt32 _srcPixelUsed : 1;
  UInt32 _dstPixelUsed : 1;

  UInt32 _srcAlphaUsed : 1;
  UInt32 _dstAlphaUsed : 1;
};

//! @}

} // BlitJit namespace

// [Guard]
#endif // _BLITJITDEFS_H
