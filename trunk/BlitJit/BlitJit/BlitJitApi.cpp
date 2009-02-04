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

// [Dependencies]
#include "BlitJitApi.h"

namespace BlitJit {

const PixelFormat Api::pixelFormats[PixelFormat::Count] = 
{
  // Name   | Id                 | D | rMask     | gMask     | bMask     | aMask     | rgbaShift       | rgbaBytePos   | premul |
  { "ARGB32", PixelFormat::ARGB32, 32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000, 16,  8 ,  0 , 24, 2 , 1 , 0 , 3 , false  },
  { "PRGB32", PixelFormat::PRGB32, 32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000, 16,  8 ,  0 , 24, 2 , 1 , 0 , 3 , true   },
  { "XRGB32", PixelFormat::XRGB32, 32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0x00000000, 16,  8 ,  0 ,  0, 2 , 1 , 0 , 3 , false  },

  { "RGB24" , PixelFormat::RGB24 , 24, 0x00FF0000, 0x0000FF00, 0x000000FF, 0x00000000, 16,  8 ,  0 ,  0, 2 , 1 , 0 , 0 , false  },
  { "BGR24" , PixelFormat::BGR24 , 24, 0x000000FF, 0x0000FF00, 0x00FF0000, 0x00000000,  0,  8 , 16 ,  0, 2 , 1 , 0 , 0 , false  }
};

const Operation Api::operations[Operation::Count] = 
{
  // Name               | Id                          | S, D Pixel  | S, D Alpha   |
  { "CompineCopy"       , Operation::CombineCopy      , true , false, false, false },
  { "CompineBlend"      , Operation::CombineBlend     , true , false, false, false }
};

} // BlitJit namespace
