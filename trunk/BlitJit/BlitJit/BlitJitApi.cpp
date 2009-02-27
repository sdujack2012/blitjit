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
  // Name   | Id                 | D | RGBA Size     | RGBA Shift      | premul |
  { "ARGB32", PixelFormat::ARGB32, 32,  8,  8,  8,  8, 16,  8 ,  0 , 24, false  },
  { "PRGB32", PixelFormat::PRGB32, 32,  8,  8,  8,  8, 16,  8 ,  0 , 24, true   },
  { "XRGB32", PixelFormat::XRGB32, 32,  8,  8,  8,  0, 16,  8 ,  0 ,  0, false  },

  { "RGB24" , PixelFormat::RGB24 , 24,  8,  8,  8,  0, 16,  8 ,  0 ,  0, false  },
  { "BGR24" , PixelFormat::BGR24 , 24,  8,  8,  8,  0,  0,  8 , 16 ,  0, false  }
};

const Operation Api::operations[Operation::Count] = 
{
  // Name                 | Id                             | S, D Pixel  | S, D Alpha
  { "CompositeSrc"        , Operation::CompositeSrc        , true , true , false, false  },
  { "CompositeDest"       , Operation::CompositeDest       , false, false, true , true   },
  { "CompositeOver"       , Operation::CompositeOver       , true , true , true , true   },
  { "CompositeOverReverse", Operation::CompositeOverReverse, true , true , true , true   },
  { "CompositeIn"         , Operation::CompositeIn         , true , true , true , true   },
  { "CompositeInReverse"  , Operation::CompositeInReverse  , true , true , true , true   },
  { "CompositeOut"        , Operation::CompositeOut        , true , true , true , true   },
  { "CompositeOutReverse" , Operation::CompositeOutReverse , true , true , true , true   },
  { "CompositeAtop"       , Operation::CompositeAtop       , true , true , true , true   },
  { "CompositeAtopReverse", Operation::CompositeAtopReverse, true , true , true , true   },
  { "CompositeXor"        , Operation::CompositeXor        , true , true , true , true   },
  { "CompositeClear"      , Operation::CompositeClear      , false, false, false, false  },
  { "CompositeAdd"        , Operation::CompositeAdd        , true , true , true , true   },
  { "CompositeSubtract"   , Operation::CompositeSubtract   , true , true , true , true   },
  { "CompositeMultiply"   , Operation::CompositeMultiply   , true , true , true , true   },
  { "CompositeSaturate"   , Operation::CompositeSaturate   , true , true , true , true   }
};

} // BlitJit namespace
