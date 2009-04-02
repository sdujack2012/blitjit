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
#include "Api.h"

namespace BlitJit {

// ============================================================================
// [Internal prototypes]
// ============================================================================

static Api::Constants* initConstants();

// ============================================================================
// [BlitJit::Api - Initialization / Deinitialization]
// ============================================================================

void Api::init()
{
  if (constants == NULL) constants = initConstants();
}

// ============================================================================
// [BlitJit::Api - Pixel Formats]
// ============================================================================

const PixelFormat Api::pixelFormats[PixelFormat::Count] = 
{
  // Name   | Id                 | D | RGBA Size     | RGBA Shift      | premul | float |
  { "ARGB32", PixelFormat::ARGB32, 32,  8,  8,  8,  8, 16,  8 ,  0 , 24, false  , false },
  { "PRGB32", PixelFormat::PRGB32, 32,  8,  8,  8,  8, 16,  8 ,  0 , 24, true   , false },
  { "XRGB32", PixelFormat::XRGB32, 32,  8,  8,  8,  0, 16,  8 ,  0 ,  0, false  , false },

  { "RGB24" , PixelFormat::RGB24 , 24,  8,  8,  8,  0, 16,  8 ,  0 ,  0, false  , false },
  { "BGR24" , PixelFormat::BGR24 , 24,  8,  8,  8,  0,  0,  8 , 16 ,  0, false  , false }
};

// ============================================================================
// [BlitJit::Api - Operators]
// ============================================================================

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
  { "CompositeScreen"     , Operation::CompositeScreen     , true , true , true , true   },
  { "CompositeDarken"     , Operation::CompositeDarken     , true , true , true , true   },
  { "CompositeLighten"    , Operation::CompositeLighten    , true , true , true , true   },
  { "CompositeDifference" , Operation::CompositeDifference , true , true , true , true   },
  { "CompositeExclusion"  , Operation::CompositeExclusion  , true , true , true , true   },
  { "CompositeInvert"     , Operation::CompositeInvert     , true , true , true , true   },
  { "CompositeInvertRgb"  , Operation::CompositeInvertRgb  , true , true , true , true   },
  { "CompositeSaturate"   , Operation::CompositeSaturate   , true , true , true , true   }
};

// ============================================================================
// [BlitJit::Api - Constants]
// ============================================================================

Api::Constants* Api::constants = NULL;

static Api::Constants* initConstants()
{
  static UInt8 constantsStorage[sizeof(Api::Constants) + 16];

  // Align to 16 bytes (default SSE alignment)
  Api::Constants* c = (Api::Constants*)(void*)(((SysUInt)constantsStorage + 15) & ~(SysUInt)15);

  c->Cx00800080008000800080008000800080.set_uw(0x0080, 0x0080, 0x0080, 0x0080, 0x0080, 0x0080, 0x0080, 0x0080);
  c->Cx00FF00FF00FF00FF00FF00FF00FF00FF.set_uw(0x00FF, 0x00FF, 0x00FF, 0x00FF, 0x00FF, 0x00FF, 0x00FF, 0x00FF);

  c->Cx000000FF00FF00FF000000FF00FF00FF.set_uw(0x00FF, 0x00FF, 0x00FF, 0x0000, 0x00FF, 0x00FF, 0x00FF, 0x0000);
  c->Cx00FF000000FF00FF00FF000000FF00FF.set_uw(0x00FF, 0x00FF, 0x0000, 0x00FF, 0x00FF, 0x00FF, 0x0000, 0x00FF);
  c->Cx00FF00FF000000FF00FF00FF000000FF.set_uw(0x00FF, 0x0000, 0x00FF, 0x00FF, 0x00FF, 0x0000, 0x00FF, 0x00FF);
  c->Cx00FF00FF00FF000000FF00FF00FF0000.set_uw(0x0000, 0x00FF, 0x00FF, 0x00FF, 0x0000, 0x00FF, 0x00FF, 0x00FF);

  c->Cx00FF00000000000000FF000000000000.set_uw(0x0000, 0x0000, 0x0000, 0x00FF, 0x0000, 0x0000, 0x0000, 0x00FF);
  c->Cx000000FF00000000000000FF00000000.set_uw(0x0000, 0x0000, 0x00FF, 0x0000, 0x0000, 0x0000, 0x00FF, 0x0000);
  c->Cx0000000000FF00000000000000FF0000.set_uw(0x0000, 0x00FF, 0x0000, 0x0000, 0x0000, 0x00FF, 0x0000, 0x0000);
  c->Cx00000000000000FF00000000000000FF.set_uw(0x00FF, 0x0000, 0x0000, 0x0000, 0x00FF, 0x0000, 0x0000, 0x0000);

  SysInt i;

  for (i = 0; i < 256; i++)
  {
    UInt16 x = i > 0 ? (int)((256.0 * 255.0) / (float)i + 0.5) : 0;
    c->CxDemultiply[i].set_uw(i, i, i, i, i, i, i, i);
  }

  return c;
}

} // BlitJit namespace
