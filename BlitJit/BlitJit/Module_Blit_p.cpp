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
#include <AsmJit/Compiler.h>
#include <AsmJit/CpuInfo.h>

#include "BlitJit.h"
#include "Constants_p.h"
#include "Generator_p.h"
#include "Module_p.h"
#include "Module_Blit_p.h"

using namespace AsmJit;

namespace BlitJit {

// ============================================================================
// [BlitJit::Module_Blit_32_SSE2]
// ============================================================================

Module_Blit_32_SSE2::Module_Blit_32_SSE2(
  Generator* g,
  const PixelFormat* dstPf,
  const PixelFormat* srcPf,
  const PixelFormat* mskPf,
  const Operator* op) :
    Module_Blit(g, dstPf, srcPf, mskPf, op)
{
  // Calculate correct pixel format positions
  dstAlphaPos = getARGB32AlphaPos(dstPf);
  srcAlphaPos = getARGB32AlphaPos(srcPf);

  // First look at NOPs
  if (op->id() == Operator::CompositeDest)
  {
    _isNop = true;
    return;
  }

  _maxPixelsPerLoop = 4;
  _complexity = Complex;

  switch (op->id())
  {
    case Operator::CompositeSubtract  : _maxPixelsPerLoop = 2; break;
    case Operator::CompositeMultiply  : _maxPixelsPerLoop = 2; break;
    case Operator::CompositeScreen    : _maxPixelsPerLoop = 2; break;
    case Operator::CompositeDarken    : _maxPixelsPerLoop = 2; break;
    case Operator::CompositeLighten   : _maxPixelsPerLoop = 2; break;
    case Operator::CompositeDifference: _maxPixelsPerLoop = 2; break;
    case Operator::CompositeExclusion : _maxPixelsPerLoop = 2; break;
    case Operator::CompositeInvert    : _maxPixelsPerLoop = 2; break;
    case Operator::CompositeInvertRgb : _maxPixelsPerLoop = 2; break;
  }
}

Module_Blit_32_SSE2::~Module_Blit_32_SSE2()
{
}

void Module_Blit_32_SSE2::init()
{
  g->usingConstants();
  g->usingXMMZero();
  g->usingXMM0080();
}

void Module_Blit_32_SSE2::free()
{
}

void Module_Blit_32_SSE2::processPixelsPtr(
  const AsmJit::PtrRef* dst,
  const AsmJit::PtrRef* src,
  const AsmJit::PtrRef* msk,
  SysInt count,
  SysInt offset,
  UInt32 kind,
  UInt32 flags)
{
  StateRef state(c->saveState());

  // These are needed in all cases
  XMMRef dstpix0(c->newVariable(VARIABLE_TYPE_XMM, 5));
  XMMRef srcpix0(c->newVariable(VARIABLE_TYPE_XMM, 5));

  bool srcAligned = (flags & SrcAligned) != 0;
  bool dstAligned = (flags & DstAligned) != 0;

  SysInt dstDisp = dstPf->bytesPerPixel() * offset;
  SysInt srcDisp = srcPf->bytesPerPixel() * offset;

  switch (count)
  {
    case 1: // Process 1 pixel
    {
      Label* L_LocalLoopExit = c->newLabel();

      // This can improve speed by skipping pixels of zero or full alpha
      if (op->id() == Operator::CompositeOver)
      {
        SysIntRef srcpix0i(c->newVariable(VARIABLE_TYPE_SYSINT, 5));
        Label* L_1 = c->newLabel();

        c->mov(srcpix0i.x32(), ptr(src->r(), dstDisp));
        c->cmp(srcpix0i.c32(), imm(0));
        c->jz(L_LocalLoopExit, HINT_NOT_TAKEN);

        c->movd(srcpix0.x(), srcpix0i.c32());
        c->movd(dstpix0.x(), ptr(dst->r(), srcDisp));
      }
      else
      {
        c->movd(srcpix0.x(), ptr(src->r(), dstDisp));
        c->movd(dstpix0.x(), ptr(dst->r(), srcDisp));
      }

      processPixelsRaw(dstpix0, srcpix0, false);
      c->movd(ptr(dst->r(), dstDisp), dstpix0.r());

      c->bind(L_LocalLoopExit);
      break;
    }
    case 2: // Process 2 pixels
    {
      c->movq(srcpix0.x(), ptr(src->r(), srcDisp));
      c->movq(dstpix0.x(), ptr(dst->r(), dstDisp));
      processPixelsRaw(dstpix0, srcpix0, true);
      c->movq(ptr(dst->r(), dstDisp), dstpix0.r());
      break;
    }
    case 4: // Process 4 pixels
    {
      if (op->id() == Operator::CompositeOver)
      {
        // Need more variables to successfully parallelize instructions.
        XMMRef dstpix1(c->newVariable(VARIABLE_TYPE_XMM, 5));
        XMMRef srcpix1(c->newVariable(VARIABLE_TYPE_XMM, 5));
        XMMRef t0(c->newVariable(VARIABLE_TYPE_XMM, 5));
        XMMRef t1(c->newVariable(VARIABLE_TYPE_XMM, 5));
        Int32Ref srcpixi(c->newVariable(VARIABLE_TYPE_INT32));

        Label* L_LocalLoopExit = c->newLabel();
        Label* L_LocalLoopStore = c->newLabel();

        g->loadDQ(srcpix0, ptr(src->r(), srcDisp), srcAligned);

        c->pcmpeqb(dstpix0.r(), dstpix0.r());
        c->pxor(dstpix1.r(), dstpix1.r());

        c->pcmpeqb(dstpix0.r(), srcpix0.r());
        c->pcmpeqb(dstpix1.r(), srcpix0.r());

        {
          SysIntRef t(c->newVariable(VARIABLE_TYPE_SYSINT, 0));
          SysIntRef k(c->newVariable(VARIABLE_TYPE_SYSINT, 0));

          c->pmovmskb(k.r32(), dstpix0.r());
          c->pmovmskb(t.r32(), dstpix1.r());

          c->and_(k.r32(), 0x8888);
          c->cmp(t.r32(), 0xFFFF);
          c->jz(L_LocalLoopExit);

          c->cmp(k.r32(), 0x8888);
          c->jz(L_LocalLoopStore);
        }

        g->loadDQ(dstpix0, ptr(dst->r(), dstDisp), dstAligned);

        c->movdqa(srcpix1.r(), srcpix0.r());
        c->punpcklbw(srcpix0.r(), g->xmmZero().r());

        c->movdqa(dstpix1.r(), dstpix0.r());
        c->punpckhbw(srcpix1.r(), g->xmmZero().r());

        c->punpcklbw(dstpix0.r(), g->xmmZero().r());
        c->punpckhbw(dstpix1.r(), g->xmmZero().r());

        g->extractAlpha_2x2W_SSE2(t0, srcpix0, dstAlphaPos, true, t1, srcpix1, dstAlphaPos, true);

        g->mul_2x2W_SSE2(dstpix0, dstpix0, t0, dstpix1, dstpix1, t1);
        g->add_2x2W_SSE2(srcpix0, srcpix0, dstpix0, srcpix1, srcpix1, dstpix1);

        c->packuswb(srcpix0.r(), srcpix1.r());

        c->bind(L_LocalLoopStore);
        g->storeDQ(ptr(dst->r(), dstDisp), srcpix0, false, dstAligned);
        c->bind(L_LocalLoopExit);
      }
      else
      {
        // Need more variables to successfully parallelize instructions.
        XMMRef dstpix1(c->newVariable(VARIABLE_TYPE_XMM, 5));
        XMMRef srcpix1(c->newVariable(VARIABLE_TYPE_XMM, 5));

        Label* L_LocalLoopExit = c->newLabel();

        g->loadDQ(srcpix0, ptr(src->r(), srcDisp), srcAligned);


        g->loadDQ(dstpix0, ptr(dst->r(), dstDisp), dstAligned);

        // Source and destination is in srcpix0 and dstpix0, also we want to
        // pack destination to dstpix0.
        processPixelsRaw_4(dstpix0, srcpix0, dstpix1, srcpix1,
          Module_Blit_32_SSE2::Raw4UnpackFromSrc0 |
          Module_Blit_32_SSE2::Raw4UnpackFromDst0 |
          Module_Blit_32_SSE2::Raw4PackToDst0);

        g->storeDQ(ptr(dst->r(), dstDisp), dstpix0, false, dstAligned);

        c->bind(L_LocalLoopExit);
      }
      break;
    }
  }
}

void Module_Blit_32_SSE2::processPixelsRaw(
  const XMMRef& dst0, const XMMRef& src0,
  bool two)
{
  switch (op->id())
  {
    case Operator::CompositeSrc:
      // copy operation (optimized in frontends and also by Generator itself)
      c->movdqa(dst0.r(), src0.r());
      return;
    case Operator::CompositeDest:
      // no operation (optimized in frontends and also by Generator itself)
      return;
    case Operator::CompositeClear:
      // clear operation (optimized in frontends and also by Generator itself)
      c->pxor(dst0.r(), dst0.r());
      return;
    case Operator::CompositeAdd:
      // add operation (not needs to be unpacked and packed)
      c->paddusb(dst0.r(), src0.r());
      return;
    default:
      // default operations - needs unpacking before and packing after
      break;
  }

  c->punpcklbw(src0.r(), g->xmmZero().r());
  c->punpcklbw(dst0.r(), g->xmmZero().r());

  g->composite_1x1W_SSE2(dst0, src0, dstAlphaPos, op, two);

  c->packuswb(dst0.r(), dst0.r());
}

void Module_Blit_32_SSE2::processPixelsRaw_4(
  const XMMRef& dst0, const XMMRef& src0,
  const XMMRef& dst1, const XMMRef& src1,
  UInt32 flags)
{
  if (flags & Raw4UnpackFromSrc0) c->movdqa(src1.r(), src0.r());
  if (flags & Raw4UnpackFromDst0) c->movdqa(dst1.r(), dst0.r());

  if (flags & Raw4UnpackFromSrc0)
  {
    c->punpcklbw(src0.r(), g->xmmZero().r());
    c->punpckhbw(src1.r(), g->xmmZero().r());
  }
  else
  {
    c->punpcklbw(src0.r(), g->xmmZero().r());
    c->punpcklbw(src1.r(), g->xmmZero().r());
  }

  if (flags & Raw4UnpackFromDst0)
  {
    c->punpcklbw(dst0.r(), g->xmmZero().r());
    c->punpckhbw(dst1.r(), g->xmmZero().r());
  }
  else
  {
    c->punpcklbw(dst0.r(), g->xmmZero().r());
    c->punpcklbw(dst1.r(), g->xmmZero().r());
  }

  g->composite_2x2W_SSE2(
    dst0, src0, dstAlphaPos,
    dst1, src1, dstAlphaPos,
    op);

  if (flags & Raw4PackToDst0)
  {
    c->packuswb(dst0.r(), dst1.r());
  }
  else
  {
    c->packuswb(dst0.r(), dst0.r());
    c->packuswb(dst1.r(), dst1.r());
  }
}

} // BlitJit namespace
