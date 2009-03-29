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
#ifndef _BLITJITAPI_H
#define _BLITJITAPI_H

// [Dependencies]
#include <AsmJit/AsmJitUtil.h>

#include "BlitJitBuild.h"
#include "BlitJitDefs.h"

namespace BlitJit {

//! @addtogroup BlitJit_Main
//! @{

struct BLITJIT_API Api
{
  // --------------------------------------------------------------------------
  // [Initialization / Deinitialization]
  // --------------------------------------------------------------------------

  static void init();

  // --------------------------------------------------------------------------
  // [Pixel Formats]
  // --------------------------------------------------------------------------

  static const PixelFormat pixelFormats[PixelFormat::Count];

  // --------------------------------------------------------------------------
  // [Operators]
  // --------------------------------------------------------------------------

  static const Operation operations[Operation::Count];

  // --------------------------------------------------------------------------
  // [Constants]
  // --------------------------------------------------------------------------

  //! @brief Constants singleton that generated functions can use.
  //!
  //! There is only one allocated instance for all constants. Constants are
  //! always 16 byte aligned so they can be used through SSE2 instructions.
  struct Constants
  {
    AsmJit::XMMData Cx00800080008000800080008000800080; // [0]
    AsmJit::XMMData Cx00FF00FF00FF00FF00FF00FF00FF00FF; // [1]
    
    AsmJit::XMMData Cx000000FF00FF00FF000000FF00FF00FF; // [2]
    AsmJit::XMMData Cx00FF000000FF00FF00FF000000FF00FF; // [3]
    AsmJit::XMMData Cx00FF00FF000000FF00FF00FF000000FF; // [4]
    AsmJit::XMMData Cx00FF00FF00FF000000FF00FF00FF0000; // [5]

    AsmJit::XMMData Cx00FF00000000000000FF000000000000; // [6]
    AsmJit::XMMData Cx000000FF00000000000000FF00000000; // [7]
    AsmJit::XMMData Cx0000000000FF00000000000000FF0000; // [8]
    AsmJit::XMMData Cx00000000000000FF00000000000000FF; // [9]
    
    AsmJit::XMMData CxDemultiply[256];
  };

  static Constants* constants;
};

//! @}

} // BlitJit namespace

// [Guard]
#endif // _BLITJITAPI_H
