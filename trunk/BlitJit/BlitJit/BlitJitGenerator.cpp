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

#include "BlitJitApi.h"
#include "BlitJitGenerator.h"

#include <AsmJit/AsmJitAssembler.h>
#include <AsmJit/AsmJitCompiler.h>
#include <AsmJit/AsmJitCpuInfo.h>

#if 0 && defined(ASMJIT_X64)
#define UNROLL_8
#endif // ASMJIT_X64

using namespace AsmJit;

namespace BlitJit {

// ============================================================================
// [BlitJit::Macros]
// ============================================================================

#define RCONST_DISP(__cname__) (Int32)( (UInt8 *)&Generator::constants->__cname__ - (UInt8 *)Generator::constants )

// ============================================================================
// [BlitJit::GeneratorOp]
// ============================================================================

struct GeneratorOp
{
  GeneratorOp(Generator* g, Compiler* c, Function* f) : g(g), c(c), f(f) {}
  virtual ~GeneratorOp() {}

  virtual void init() {};
  virtual void free() {};

  Generator* g;
  Compiler* c;
  Function* f;
};

// ============================================================================
// [BlitJit::CompositeOp32]
// ============================================================================

//! @brief Implementation of compositing operations for SSE2 instruction set.
struct GeneratorOpComposite32_SSE2 : public GeneratorOp
{
  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  GeneratorOpComposite32_SSE2(Generator* g, Compiler* c, Function* f, UInt32 op) : GeneratorOp(g, c, f), op(op), unroll(1)
  {
    unroll = 0;
    unpackUsingZ = 1;

    if (op == Operation::CompositeSaturate) unroll = 0;
  }

  virtual ~GeneratorOpComposite32_SSE2() {}

  enum RawFlags
  {
    Raw4UnpackFromDst0 = (1 << 0),
    Raw4UnpackFromDst2 = (1 << 1), // X64
    Raw4UnpackFromSrc0 = (1 << 2),
    Raw4UnpackFromSrc2 = (1 << 3), // X64
    Raw4PackToDst0 = (1 << 4),
    Raw4PackToDst2 = (1 << 5) // X64
  };

  // --------------------------------------------------------------------------
  // [init / free]
  // --------------------------------------------------------------------------

  virtual void init()
  {
    g->initRConst();

    z.use(f->newVariable(VARIABLE_TYPE_XMM, 0));
    z.alloc();

    c0x0080.use(f->newVariable(VARIABLE_TYPE_XMM, 0));
    c0x0080.alloc();

    c->pxor(z.r(), z.r());
    c->movdqa(c0x0080.r(), ptr(g->rconst.r(), 
      RCONST_DISP(Cx00800080008000800080008000800080)));
  }

  virtual void free()
  {
    z.unuse();
  }

  // --------------------------------------------------------------------------
  // [doPixelUnpacked]
  // --------------------------------------------------------------------------

  void doPixelRaw(
    const XMMRegister& dst0, const XMMRegister& src0,
    bool two)
  {
    switch (op)
    {
      case Operation::CompositeSrc:
        // copy operation (optimized in frontends and also by Generator itself)
        c->movdqa(dst0, src0);
        return;
      case Operation::CompositeDest:
        // no operation (optimized in frontends and also by Generator itself)
        return;
      case Operation::CompositeClear:
        // clear operation (optimized in frontends and also by Generator itself)
        c->pxor(dst0, dst0);
        return;
      case Operation::CompositeAdd:
        // add operation (not needs to be unpacked and packed)
        c->paddusb(dst0, src0);
        return;
      default:
        // default operations - needs unpacking before and packing after
        break;
    }

    c->punpcklbw(src0, unpackUsingZ ? z.r() : src0);
    c->punpcklbw(dst0, unpackUsingZ ? z.r() : dst0);

    if (!unpackUsingZ)
    {
      c->psrlw(src0, 8);
      c->psrlw(dst0, 8);
    }

    doPixelUnpacked(dst0, src0, two);
    c->packuswb(dst0, dst0);
  }

  void doPixelRaw_4(
    const XMMRegister& dst0, const XMMRegister& src0,
    const XMMRegister& dst1, const XMMRegister& src1,
    UInt32 flags)
  {
    if (flags & Raw4UnpackFromSrc0) c->movdqa(src1, src0);
    if (flags & Raw4UnpackFromDst0) c->movdqa(dst1, dst0);

    if (flags & Raw4UnpackFromSrc0)
    {
      c->punpcklbw(src0, unpackUsingZ ? z.r() : src0);
      c->punpckhbw(src1, unpackUsingZ ? z.r() : src1);
    }
    else
    {
      c->punpcklbw(src0, unpackUsingZ ? z.r() : src0);
      c->punpcklbw(src1, unpackUsingZ ? z.r() : src1);
    }

    if (flags & Raw4UnpackFromDst0)
    {
      c->punpcklbw(dst0, unpackUsingZ ? z.r() : dst0);
      c->punpckhbw(dst1, unpackUsingZ ? z.r() : dst1);
    }
    else
    {
      c->punpcklbw(dst0, unpackUsingZ ? z.r() : dst0);
      c->punpcklbw(dst1, unpackUsingZ ? z.r() : dst1);
    }

    if (!unpackUsingZ)
    {
      c->psrlw(src0, 8);
      c->psrlw(src1, 8);
      c->psrlw(dst0, 8);
      c->psrlw(dst1, 8);
    }

    doPixelUnpacked_4(dst0, src0, dst1, src1);

    if (flags & Raw4PackToDst0)
    {
      c->packuswb(dst0, dst1);
    }
    else
    {
      c->packuswb(dst0, dst0);
      c->packuswb(dst1, dst1);
    }
  }

#if defined(ASMJIT_X64)
  void doPixelRaw_8(
    const XMMRegister& dst0, const XMMRegister& src0,
    const XMMRegister& dst1, const XMMRegister& src1,
    const XMMRegister& dst2, const XMMRegister& src2,
    const XMMRegister& dst3, const XMMRegister& src3,
    UInt32 flags)
  {
    if (flags & Raw4UnpackFromSrc0) c->movdqa(src1, src0);
    if (flags & Raw4UnpackFromSrc2) c->movdqa(src3, src2);
    if (flags & Raw4UnpackFromDst0) c->movdqa(dst1, dst0);
    if (flags & Raw4UnpackFromDst2) c->movdqa(dst3, dst2);

    if (flags & Raw4UnpackFromSrc0)
    {
      c->punpcklbw(src0, unpackUsingZ ? z.r() : src0);
      c->punpckhbw(src1, unpackUsingZ ? z.r() : src1);
    }
    else
    {
      c->punpcklbw(src0, unpackUsingZ ? z.r() : src0);
      c->punpcklbw(src1, unpackUsingZ ? z.r() : src1);
    }

    if (flags & Raw4UnpackFromSrc2)
    {
      c->punpcklbw(src2, unpackUsingZ ? z.r() : src2);
      c->punpckhbw(src3, unpackUsingZ ? z.r() : src3);
    }
    else
    {
      c->punpcklbw(src2, unpackUsingZ ? z.r() : src2);
      c->punpcklbw(src3, unpackUsingZ ? z.r() : src3);
    }

    if (flags & Raw4UnpackFromDst0)
    {
      c->punpcklbw(dst0, unpackUsingZ ? z.r() : dst0);
      c->punpckhbw(dst1, unpackUsingZ ? z.r() : dst1);
    }
    else
    {
      c->punpcklbw(dst0, unpackUsingZ ? z.r() : dst0);
      c->punpcklbw(dst1, unpackUsingZ ? z.r() : dst1);
    }

    if (flags & Raw4UnpackFromDst2)
    {
      c->punpcklbw(dst2, unpackUsingZ ? z.r() : dst2);
      c->punpckhbw(dst3, unpackUsingZ ? z.r() : dst3);
    }
    else
    {
      c->punpcklbw(dst2, unpackUsingZ ? z.r() : dst2);
      c->punpcklbw(dst3, unpackUsingZ ? z.r() : dst3);
    }

    if (!unpackUsingZ)
    {
      c->psrlw(src0, 8);
      c->psrlw(src1, 8);
      c->psrlw(src2, 8);
      c->psrlw(src3, 8);
      c->psrlw(dst0, 8);
      c->psrlw(dst1, 8);
      c->psrlw(dst2, 8);
      c->psrlw(dst3, 8);
    }

    doPixelUnpacked_8(dst0, src0, dst1, src1, dst2, src2, dst3, src3);

    if (flags & Raw4PackToDst0)
    {
      c->packuswb(dst0, dst1);
    }
    else
    {
      c->packuswb(dst0, dst0);
      c->packuswb(dst1, dst1);
    }

    if (flags & Raw4PackToDst2)
    {
      c->packuswb(dst2, dst3);
    }
    else
    {
      c->packuswb(dst2, dst2);
      c->packuswb(dst3, dst3);
    }
  }
#endif // ASMJIT_X64

  // --------------------------------------------------------------------------
  // [doPixelUnpacked]
  // --------------------------------------------------------------------------

  void doPixelUnpacked(
    const XMMRegister& dst0, const XMMRegister& src0, bool two)
  {
    XMMRef _t0 = f->newVariable(VARIABLE_TYPE_XMM, 0);
    XMMRef _t1 = f->newVariable(VARIABLE_TYPE_XMM, 0);

    XMMRegister t0 = _t0.r();
    XMMRegister t1 = _t1.r();

    switch (op)
    {
      case Operation::CompositeSrc:
        // copy operation (optimized in frontends and also by Generator itself)
        c->movdqa(dst0, src0);
        break;
      case Operation::CompositeDest:
        // no operation (optimized in frontends and also by Generator itself)
        break;
      case Operation::CompositeOver:
        doExtractAlpha(t0, src0, 3, true, two);
        doPackedMultiply(dst0, t0, t0);
        c->paddusb(dst0, src0);
        break;
      case Operation::CompositeOverReverse:
        doExtractAlpha(t0, dst0, 3, true, two);
        doPackedMultiply(src0, t0, t0);
        c->paddusb(dst0, src0);
        break;
      case Operation::CompositeIn:
        doExtractAlpha(t0, dst0, 3, false, two);
        doPackedMultiply(src0, t0, dst0, true);
        break;
      case Operation::CompositeInReverse:
        doExtractAlpha(t0, src0, 3, false, two);
        doPackedMultiply(dst0, t0, t0);
        break;
      case Operation::CompositeOut:
        doExtractAlpha(t0, dst0, 3, true, two);
        doPackedMultiply(src0, t0, dst0, true);
        break;
      case Operation::CompositeOutReverse:
        doExtractAlpha(t0, src0, 3, true, two);
        doPackedMultiply(dst0, t0, t0);
        break;
      case Operation::CompositeAtop:
        doExtractAlpha(t0, src0, 3, true, two);
        doExtractAlpha(t1, dst0, 3, false, two);
        doPackedMultiplyAdd(src0, t1, dst0, t0, dst0, true);
        break;
      case Operation::CompositeAtopReverse:
        doExtractAlpha(t0, src0, 3, false, two);
        doExtractAlpha(t1, dst0, 3, true, two);
        doPackedMultiplyAdd(src0, t1, dst0, t0, dst0, true);
        break;
      case Operation::CompositeXor:
        doExtractAlpha(t0, src0, 3, true, two);
        doExtractAlpha(t1, dst0, 3, true, two);
        doPackedMultiplyAdd(src0, t1, dst0, t0, dst0, true);
        break;
      case Operation::CompositeClear:
        // clear operation (optimized in frontends and also by Generator itself)
        c->pxor(dst0, dst0);
        break;
      case Operation::CompositeAdd:
        c->paddusb(dst0, src0);
        break;
      case Operation::CompositeSubtract:
        c->psubusb(dst0, src0);
        break;
      case Operation::CompositeMultiply:
        doPackedMultiply(dst0, src0, t1);
        break;
      case Operation::CompositeSaturate:
      {
        Label* L_Skip = c->newLabel();
        Int32Ref td = f->newVariable(VARIABLE_TYPE_INT32, 0);
        Int32Ref ts = f->newVariable(VARIABLE_TYPE_INT32, 0);

        c->movdqa(t1, dst0);
        c->movdqa(t0, src0);
        c->psrlq(t1, 48);
        c->psrlq(t0, 48);
        c->movd(td.r(), t1);
        c->movd(ts.r(), t0);
        c->neg(td.r());
        c->cmp(ts.r16(), td.r16());
        c->jle(L_Skip);

        c->and_(ts.r(), 0xFF);
        c->shl(ts.r(), 4);
        c->movd(t0, td.r());
        doExtractAlpha(t0, t0, 0, 0, false);
        c->movdqa(t1, ptr(g->rconst.r(), ts.r(), 0, RCONST_DISP(CxDemultiply)));
        doPackedMultiply(t0, t1, t1);
        doPackedMultiply(src0, t0, t0);

        c->bind(L_Skip);
        c->paddusb(dst0, src0);
        break;
      }
    }
  }

  void doPixelUnpacked_4(
    const XMMRegister& dst0, const XMMRegister& src0,
    const XMMRegister& dst1, const XMMRegister& src1)
  {
    XMMRef _t0 = f->newVariable(VARIABLE_TYPE_XMM, 0);
    XMMRef _t1 = f->newVariable(VARIABLE_TYPE_XMM, 0);

    XMMRegister t0 = _t0.r();
    XMMRegister t1 = _t1.r();

    switch (op)
    {
      case Operation::CompositeSrc:
        // copy operation (optimized in frontends and also by Generator itself)
        c->movdqa(dst0, src0);
        c->movdqa(dst1, src1);
        break;
      case Operation::CompositeDest:
        // no operation (optimized in frontends and also by Generator itself)
        break;
      case Operation::CompositeOver:
        doExtractAlpha_4(
          t0, src0, 3, true,
          t1, src1, 3, true);
        doPackedMultiply_4(
          dst0, t0, t0,
          dst1, t1, t1);
        c->paddusb(dst0, src0);
        c->paddusb(dst1, src1);
        break;
      case Operation::CompositeOverReverse:
        doExtractAlpha_4(
          t0, dst0, 3, true,
          t1, dst1, 3, true);
        doPackedMultiply_4(
          src0, t0, t0,
          src1, t1, t1);
        c->paddusb(dst0, src0);
        c->paddusb(dst1, src1);
        break;
      case Operation::CompositeIn:
        doExtractAlpha_4(
          t0, dst0, 3, false,
          t1, dst1, 3, false);
        doPackedMultiply_4(
          src0, t0, dst0,
          src1, t1, dst1,
          true);
        break;
      case Operation::CompositeInReverse:
        doExtractAlpha_4(
          t0, src0, 3, false,
          t1, src1, 3, false);
        doPackedMultiply_4(
          dst0, t0, t0,
          dst1, t1, t1);
        break;
      case Operation::CompositeOut:
        doExtractAlpha_4(
          t0, dst0, 3, true,
          t1, dst1, 3, true);
        doPackedMultiply_4(
          src0, t0, dst0,
          src1, t1, dst1,
          true);
        break;
      case Operation::CompositeOutReverse:
        doExtractAlpha_4(
          t0, src0, 3, true,
          t1, src1, 3, true);
        doPackedMultiply_4(
          dst0, t0, t0,
          dst1, t1, t1);
        break;
      case Operation::CompositeAtop:
        doExtractAlpha_4(
          t0, src0, 3, true,
          t1, dst0, 3, false);
        doPackedMultiplyAdd(src0, t1, dst0, t0, dst0, true);

        doExtractAlpha_4(
          t0, src1, 3, true,
          t1, dst1, 3, false);
        doPackedMultiplyAdd(src1, t1, dst1, t0, dst1, true);
        break;
      case Operation::CompositeAtopReverse:
        doExtractAlpha_4(
          t0, src0, 3, false,
          t1, dst0, 3, true);
        doPackedMultiplyAdd(src0, t1, dst0, t0, dst0, true);

        doExtractAlpha_4(
          t0, src1, 3, false,
          t1, dst1, 3, true);
        doPackedMultiplyAdd(src1, t1, dst1, t0, dst1, true);
        break;
      case Operation::CompositeXor:
        doExtractAlpha_4(
          t0, src0, 3, true,
          t1, dst0, 3, true);
        doPackedMultiplyAdd(src0, t1, dst0, t0, dst0, true);

        doExtractAlpha_4(
          t0, src1, 3, true,
          t1, dst1, 3, true);
        doPackedMultiplyAdd(src1, t1, dst1, t0, dst1, true);
        break;
      case Operation::CompositeClear:
        // clear operation (optimized in frontends and also by Generator itself)
        c->pxor(dst0, dst0);
        c->pxor(dst1, dst1);
        break;
      case Operation::CompositeAdd:
        c->paddusb(dst0, src0);
        c->paddusb(dst1, src1);
        break;
      case Operation::CompositeSubtract:
        c->psubusb(dst0, src0);
        c->psubusb(dst1, src1);
        break;
      case Operation::CompositeMultiply:
        doPackedMultiply_4(
          dst0, src0, t0,
          dst1, src1, t1);
        break;
      case Operation::CompositeSaturate:
      {
        ASMJIT_ASSERT(0);
        break;
      }
    }
  }

#if defined(ASMJIT_X64)
  void doPixelUnpacked_8(
    const XMMRegister& dst0, const XMMRegister& src0,
    const XMMRegister& dst1, const XMMRegister& src1,
    const XMMRegister& dst2, const XMMRegister& src2,
    const XMMRegister& dst3, const XMMRegister& src3)
  {
    XMMRef _t0 = f->newVariable(VARIABLE_TYPE_XMM, 0);
    XMMRef _t1 = f->newVariable(VARIABLE_TYPE_XMM, 0);
    XMMRef _t2 = f->newVariable(VARIABLE_TYPE_XMM, 0);
    XMMRef _t3 = f->newVariable(VARIABLE_TYPE_XMM, 0);

    XMMRegister t0 = _t0.r();
    XMMRegister t1 = _t1.r();
    XMMRegister t2 = _t2.r();
    XMMRegister t3 = _t3.r();

    switch (op)
    {
      case Operation::CompositeSrc:
        // copy operation (optimized in frontends and also by Generator itself)
        c->movdqa(dst0, src0);
        c->movdqa(dst1, src1);
        c->movdqa(dst2, src2);
        c->movdqa(dst3, src3);
        break;
      case Operation::CompositeDest:
        // no operation (optimized in frontends and also by Generator itself)
        break;
      case Operation::CompositeOver:
        doExtractAlpha_8(
          t0, src0, 3, true,
          t1, src1, 3, true,
          t2, src2, 3, true,
          t3, src3, 3, true);
        doPackedMultiply_8(
          dst0, t0, t0,
          dst1, t1, t1,
          dst2, t2, t2,
          dst3, t3, t3);
        c->paddusb(dst0, src0);
        c->paddusb(dst1, src1);
        c->paddusb(dst2, src2);
        c->paddusb(dst3, src3);
        break;
      case Operation::CompositeOverReverse:
        doExtractAlpha_8(
          t0, dst0, 3, true,
          t1, dst1, 3, true,
          t2, dst2, 3, true,
          t3, dst3, 3, true);
        doPackedMultiply_8(
          src0, t0, t0,
          src1, t1, t1,
          src2, t2, t2,
          src3, t3, t3);
        c->paddusb(dst0, src0);
        c->paddusb(dst1, src1);
        c->paddusb(dst2, src2);
        c->paddusb(dst3, src3);
        break;
      case Operation::CompositeIn:
        doExtractAlpha_8(
          t0, dst0, 3, false,
          t1, dst1, 3, false,
          t2, dst2, 3, false,
          t3, dst3, 3, false);
        doPackedMultiply_8(
          src0, t0, dst0,
          src1, t1, dst1,
          src2, t2, dst2,
          src3, t3, dst3,
          true);
        break;
      case Operation::CompositeInReverse:
        doExtractAlpha_8(
          t0, src0, 3, false,
          t1, src1, 3, false,
          t2, src2, 3, false,
          t3, src3, 3, false);
        doPackedMultiply_8(
          dst0, t0, t0,
          dst1, t1, t1,
          dst2, t2, t2,
          dst3, t3, t3);
        break;
      case Operation::CompositeOut:
        doExtractAlpha_8(
          t0, dst0, 3, true,
          t1, dst1, 3, true,
          t2, dst2, 3, true,
          t3, dst3, 3, true);
        doPackedMultiply_8(
          src0, t0, dst0,
          src1, t1, dst1,
          src2, t2, dst2,
          src3, t3, dst3,
          true);
        break;
      case Operation::CompositeOutReverse:
        doExtractAlpha_8(
          t0, src0, 3, true,
          t1, src1, 3, true,
          t2, src2, 3, true,
          t3, src3, 3, true);
        doPackedMultiply_8(
          dst0, t0, t0,
          dst1, t1, t1,
          dst2, t2, t2,
          dst3, t3, t3);
        break;
      case Operation::CompositeAtop:
        doExtractAlpha_8(
          t0, src0, 3, true ,
          t1, dst0, 3, false,
          t2, src1, 3, true ,
          t3, dst1, 3, false);
        doPackedMultiplyAdd_4(
          src0, t1, dst0, t0, dst0,
          src1, t3, dst1, t2, dst1,
          true);
        doExtractAlpha_8(
          t0, src2, 3, true ,
          t1, dst2, 3, false,
          t2, src3, 3, true ,
          t3, dst3, 3, false);
        doPackedMultiplyAdd_4(
          src2, t1, dst2, t0, dst2,
          src3, t3, dst3, t2, dst3,
          true);
        break;
      case Operation::CompositeAtopReverse:
        doExtractAlpha_8(
          t0, src0, 3, false,
          t1, dst0, 3, true ,
          t2, src1, 3, false,
          t3, dst1, 3, true );
        doPackedMultiplyAdd_4(
          src0, t1, dst0, t0, dst0,
          src1, t3, dst1, t2, dst1,
          true);
        doExtractAlpha_8(
          t0, src2, 3, false,
          t1, dst2, 3, true ,
          t2, src3, 3, false,
          t3, dst3, 3, true );
        doPackedMultiplyAdd_4(
          src2, t1, dst2, t0, dst2,
          src3, t3, dst3, t2, dst3,
          true);
        break;
      case Operation::CompositeXor:
        doExtractAlpha_8(
          t0, src0, 3, true,
          t1, dst0, 3, true,
          t2, src1, 3, true,
          t3, dst1, 3, true);
        doPackedMultiplyAdd_4(
          src0, t1, dst0, t0, dst0,
          src1, t3, dst1, t2, dst1,
          true);
        doExtractAlpha_8(
          t0, src2, 3, true,
          t1, dst2, 3, true,
          t2, src3, 3, true,
          t3, dst3, 3, true);
        doPackedMultiplyAdd_4(
          src2, t1, dst2, t0, dst2,
          src3, t3, dst3, t2, dst3,
          true);
        break;
      case Operation::CompositeClear:
        // clear operation (optimized in frontends and also by Generator itself)
        c->pxor(dst0, dst0);
        c->pxor(dst1, dst1);
        c->pxor(dst2, dst2);
        c->pxor(dst3, dst3);
        break;
      case Operation::CompositeAdd:
        c->paddusb(dst0, src0);
        c->paddusb(dst1, src1);
        c->paddusb(dst2, src2);
        c->paddusb(dst3, src3);
        break;
      case Operation::CompositeSubtract:
        c->psubusb(dst0, src0);
        c->psubusb(dst1, src1);
        c->psubusb(dst2, src2);
        c->psubusb(dst3, src3);
        break;
      case Operation::CompositeMultiply:
        doPackedMultiply_8(
          dst0, src0, t0,
          dst1, src1, t1,
          dst2, src2, t2,
          dst3, src3, t3);
        break;
      case Operation::CompositeSaturate:
      {
        ASMJIT_ASSERT(0);
        break;
      }
    }
  }
#endif // ASMJIT_X64

  // --------------------------------------------------------------------------
  // [Helpers]
  // --------------------------------------------------------------------------

  //! @brief Extract alpha channel.
  //! @param dst0 Destination XMM register (can be same as @a src).
  //! @param src0 Source XMM register.
  //! @param packed Whether extract alpha for packed pixels (two pixels, 
  //! one extra instruction).
  //! @param alphaPos Alpha position.
  //! @param negate Whether to negate extracted alpha values (255 - alpha).
  void doExtractAlpha(
    const XMMRegister& dst0, const XMMRegister& src0, UInt8 alphaPos0, UInt8 negate0, bool two)
  {
    c->pshuflw(dst0, src0, mm_shuffle(alphaPos0, alphaPos0, alphaPos0, alphaPos0));
    if (two) c->pshufhw(dst0, dst0, mm_shuffle(alphaPos0, alphaPos0, alphaPos0, alphaPos0));

    if (negate0) c->pxor(dst0, ptr(g->rconst.r(), RCONST_DISP(Cx00FF00FF00FF00FF00FF00FF00FF00FF)));
  }

  void doExtractAlpha_4(
    const XMMRegister& dst0, const XMMRegister& src0, UInt8 alphaPos0, UInt8 negate0,
    const XMMRegister& dst1, const XMMRegister& src1, UInt8 alphaPos1, UInt8 negate1)
  {
    c->pshuflw(dst0, src0, mm_shuffle(alphaPos0, alphaPos0, alphaPos0, alphaPos0));
    c->pshuflw(dst1, src1, mm_shuffle(alphaPos1, alphaPos1, alphaPos1, alphaPos1));

    c->pshufhw(dst0, dst0, mm_shuffle(alphaPos0, alphaPos0, alphaPos0, alphaPos0));
    c->pshufhw(dst1, dst1, mm_shuffle(alphaPos1, alphaPos1, alphaPos1, alphaPos1));

    if (negate0) c->pxor(dst0, ptr(g->rconst.r(), RCONST_DISP(Cx00FF00FF00FF00FF00FF00FF00FF00FF)));
    if (negate1) c->pxor(dst1, ptr(g->rconst.r(), RCONST_DISP(Cx00FF00FF00FF00FF00FF00FF00FF00FF)));
  }

#if defined(ASMJIT_X64)
  void doExtractAlpha_8(
    const XMMRegister& dst0, const XMMRegister& src0, UInt8 alphaPos0, UInt8 negate0,
    const XMMRegister& dst1, const XMMRegister& src1, UInt8 alphaPos1, UInt8 negate1,
    const XMMRegister& dst2, const XMMRegister& src2, UInt8 alphaPos2, UInt8 negate2,
    const XMMRegister& dst3, const XMMRegister& src3, UInt8 alphaPos3, UInt8 negate3)
  {
    c->pshuflw(dst0, src0, mm_shuffle(alphaPos0, alphaPos0, alphaPos0, alphaPos0));
    c->pshuflw(dst1, src1, mm_shuffle(alphaPos1, alphaPos1, alphaPos1, alphaPos1));
    c->pshuflw(dst2, src2, mm_shuffle(alphaPos2, alphaPos2, alphaPos2, alphaPos2));
    c->pshuflw(dst3, src3, mm_shuffle(alphaPos3, alphaPos3, alphaPos3, alphaPos3));

    c->pshufhw(dst0, dst0, mm_shuffle(alphaPos0, alphaPos0, alphaPos0, alphaPos0));
    c->pshufhw(dst1, dst1, mm_shuffle(alphaPos1, alphaPos1, alphaPos1, alphaPos1));
    c->pshufhw(dst2, dst2, mm_shuffle(alphaPos2, alphaPos2, alphaPos2, alphaPos2));
    c->pshufhw(dst3, dst3, mm_shuffle(alphaPos3, alphaPos3, alphaPos3, alphaPos3));

    if (negate0) c->pxor(dst0, ptr(g->rconst.r(), RCONST_DISP(Cx00FF00FF00FF00FF00FF00FF00FF00FF)));
    if (negate1) c->pxor(dst1, ptr(g->rconst.r(), RCONST_DISP(Cx00FF00FF00FF00FF00FF00FF00FF00FF)));
    if (negate2) c->pxor(dst2, ptr(g->rconst.r(), RCONST_DISP(Cx00FF00FF00FF00FF00FF00FF00FF00FF)));
    if (negate3) c->pxor(dst3, ptr(g->rconst.r(), RCONST_DISP(Cx00FF00FF00FF00FF00FF00FF00FF00FF)));
  }
#endif // ASMJIT_X64

  void doPackedMultiply(
    const XMMRegister& a0, const XMMRegister& b0, const XMMRegister& t0,
    bool moveToT0 = false)
  {
    XMMRegister r = c0x0080.r();

    c->pmullw(a0, b0);          // a0 *= b0
    c->paddusw(a0, r);          // a0 += 80
    c->movdqa(t0, a0);          // t0  = a0
    c->psrlw(a0, 8);            // a0 /= 256
    if (!moveToT0)
    {
      c->paddusw(a0, t0);       // a0 += t0
      c->psrlw(a0, 8);          // a0 /= 256
    }
    else
    {
      c->paddusw(t0, a0);       // t0 += a0
      c->psrlw(t0, 8);          // t0 /= 256
    }
  }

  void doPackedMultiply_4(
    const XMMRegister& a0, const XMMRegister& b0, const XMMRegister& t0,
    const XMMRegister& a1, const XMMRegister& b1, const XMMRegister& t1,
    bool moveToT0T1 = false)
  {
    XMMRegister r = c0x0080.r();

    // Standard case
    if (t0 != t1)
    {
      c->pmullw(a0, b0);          // a0 *= b0
      c->pmullw(a1, b1);          // a1 *= b1
/*
      Low quality div

      c->psrlw(a0, 8);            // a0 /= 256
      c->psrlw(a1, 8);            // a1 /= 256
      if (moveToT0T1)
      {
        c->movdqa(t0, a0);
        c->movdqa(t1, a1);
      }
*/

      c->paddusw(a0, r);          // a0 += 80
      c->paddusw(a1, r);          // a1 += 80

      c->movdqa(t0, a0);          // t0  = a0
      c->movdqa(t1, a1);          // t1  = a1
      c->psrlw(a0, 8);            // a0 /= 256
      c->psrlw(a1, 8);            // a1 /= 256

      if (moveToT0T1)
      {
        c->paddusw(t0, a0);       // t0 += a0
        c->paddusw(t1, a1);       // t1 += a1

        c->psrlw(t0, 8);          // t0 /= 256
        c->psrlw(t1, 8);          // t1 /= 256
      }
      else
      {
        c->paddusw(a0, t0);       // a0 += t0
        c->paddusw(a1, t1);       // a1 += t1

        c->psrlw(a0, 8);          // a0 /= 256
        c->psrlw(a1, 8);          // a1 /= 256
      }

    }
    // Special case if t0 is t1 (can be used to save one regiter if you
    // haven't it)
    else
    {
      const XMMRegister& t = t0;

      // Can't be moved to t0 and t1 if they are same!
      BLITJIT_ASSERT(moveToT0T1 == 0);

      c->pmullw(a0, b0);          // a0 *= b0
      c->pmullw(a1, b1);          // a1 *= b1
      c->paddusw(a0, r);          // a0 += 80
      c->paddusw(a1, r);          // a1 += 80

      c->movdqa(t, a0);           // t   = a0
      c->psrlw(a0, 8);            // a0 /= 256
      c->paddusw(a0, t);          // a0 += t

      c->movdqa(t, a1);           // t   = a1
      c->psrlw(a1, 8);            // a1 /= 256
      c->paddusw(a1, t);          // a1 += t

      c->psrlw(a0, 8);            // a0 /= 256
      c->psrlw(a1, 8);            // a1 /= 256
    }
  }

#if defined(ASMJIT_X64)
  void doPackedMultiply_8(
    const XMMRegister& a0, const XMMRegister& b0, const XMMRegister& t0,
    const XMMRegister& a1, const XMMRegister& b1, const XMMRegister& t1,
    const XMMRegister& a2, const XMMRegister& b2, const XMMRegister& t2,
    const XMMRegister& a3, const XMMRegister& b3, const XMMRegister& t3,
    bool moveToT0T1T2T3 = false)
  {
    XMMRegister r = c0x0080.r();

    c->pmullw(a0, b0);          // a0 *= b0
    c->pmullw(a1, b1);          // a1 *= b1
    c->pmullw(a2, b2);          // a2 *= b2
    c->pmullw(a3, b3);          // a3 *= b3

    c->paddusw(a0, r);          // a0 += 80
    c->paddusw(a1, r);          // a1 += 80
    c->paddusw(a2, r);          // a2 += 80
    c->paddusw(a3, r);          // a3 += 80

    c->movdqa(t0, a0);          // t0  = a0
    c->movdqa(t1, a1);          // t1  = a1
    c->movdqa(t2, a2);          // t2  = a2
    c->movdqa(t3, a3);          // t3  = a3

    c->psrlw(a0, 8);            // a0 /= 256
    c->psrlw(a1, 8);            // a1 /= 256
    c->psrlw(a2, 8);            // a2 /= 256
    c->psrlw(a3, 8);            // a3 /= 256

    if (moveToT0T1T2T3)
    {
      c->paddusw(t0, a0);       // t0 += a0
      c->paddusw(t1, a1);       // t1 += a1
      c->paddusw(t2, a2);       // t2 += a2
      c->paddusw(t3, a3);       // t3 += a3

      c->psrlw(t0, 8);          // t0 /= 256
      c->psrlw(t1, 8);          // t1 /= 256
      c->psrlw(t2, 8);          // t2 /= 256
      c->psrlw(t3, 8);          // t3 /= 256
    }
    else
    {
      c->paddusw(a0, t0);       // a0 += t0
      c->paddusw(a1, t1);       // a1 += t1
      c->paddusw(a2, t2);       // a2 += t2
      c->paddusw(a3, t3);       // a3 += t3

      c->psrlw(a0, 8);          // a0 /= 256
      c->psrlw(a1, 8);          // a1 /= 256
      c->psrlw(a2, 8);          // a2 /= 256
      c->psrlw(a3, 8);          // a3 /= 256
    }
  }
#endif // ASMJIT_X64

  void doPackedMultiplyAdd(
    const XMMRegister& a0, const XMMRegister& b0,
    const XMMRegister& c0, const XMMRegister& d0,
    const XMMRegister& t0, bool moveToT0)
  {
    XMMRegister r = c0x0080.r();

    c->pmullw(a0, b0);          // a0 *= b0
    c->pmullw(c0, d0);          // c0 *= d0
    c->paddusw(a0, r);          // a0 += 80
    c->paddusw(a0, c0);         // a0 += c0

    c->movdqa(t0, a0);          // t0  = a0
    c->psrlw(a0, 8);            // a0 /= 256

    if (!moveToT0)
    {
      c->paddusw(a0, t0);       // a0 += t0
      c->psrlw(a0, 8);          // a0 /= 256
    }
    else
    {
      c->paddusw(t0, a0);       // t0 += a0
      c->psrlw(t0, 8);          // t0 /= 256
    }
  }

  void doPackedMultiplyAdd_4(
    const XMMRegister& a0, const XMMRegister& b0,
    const XMMRegister& c0, const XMMRegister& d0,
    const XMMRegister& t0,
    const XMMRegister& e0, const XMMRegister& f0,
    const XMMRegister& g0, const XMMRegister& h0,
    const XMMRegister& t1,
    bool moveToT0T1)
  {
    XMMRegister r = c0x0080.r();

    c->pmullw(a0, b0);          // a0 *= b0
    c->pmullw(c0, d0);          // c0 *= d0
    c->pmullw(e0, f0);          // e0 *= f0
    c->pmullw(g0, h0);          // g0 *= h0

    c->paddusw(a0, r);          // a0 += 80
    c->paddusw(e0, r);          // e0 += 80
    c->paddusw(a0, c0);         // a0 += c0
    c->paddusw(e0, g0);         // e0 += g0

    c->movdqa(t0, a0);          // t0  = a0
    c->movdqa(t1, e0);          // t1  = e0

    c->psrlw(a0, 8);            // a0 /= 256
    c->psrlw(e0, 8);            // e0 /= 256

    if (!moveToT0T1)
    {
      c->paddusw(a0, t0);       // a0 += t0
      c->paddusw(e0, t1);       // e0 += t1
      c->psrlw(a0, 8);          // a0 /= 256
      c->psrlw(e0, 8);          // e0 /= 256
    }
    else
    {
      c->paddusw(t0, a0);       // t0 += a0
      c->paddusw(t1, e0);       // t1 += e0
      c->psrlw(t0, 8);          // t0 /= 256
      c->psrlw(t1, 8);          // t1 /= 256
    }
  }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  UInt32 op;
  UInt32 unroll;
  UInt32 unpackUsingZ;

  XMMRef z;
  XMMRef c0x0080;
};

/*
struct CompositeOp32_Over : public CompositeOp32_Base
{
  CompositeOp32_Over(Generator* g, Compiler* c, Function* f) : 
    CompositeOp32_Base(g, c, f) {}
  virtual ~CompositeOp32_Over() {}

  virtual void doPixelUnpacked1(
    const XMMRegister& dst0, const XMMRegister& src0)
  {
    XMMRef _a0 = f->newVariable(VARIABLE_TYPE_XMM, 0);
    XMMRegister a0 = _a0.r();

    // expand a0
    c->pshuflw(a0, src0, mm_shuffle(3, 3, 3, 3));
    c->paddw(a0, c0x0080.r());

    // composite over
    c->psubw(src0, dst0);       // src0 -= dst0
    c->pmullw(src0, a0);        // src0 *= a0
    c->psllw(dst0, 8);          // dst0 *= 256
    c->paddw(dst0, src0);       // dst0 += src0
    c->psrlw(dst0, 8);          // dst0 /= 256
  }

  virtual void doPixelUnpacked2(
    const XMMRegister& dst0, const XMMRegister& src0)
  {
    XMMRef _a0 = f->newVariable(VARIABLE_TYPE_XMM, 0);
    XMMRegister a0 = _a0.r();

    // expand a0
    c->pshuflw(a0, src0, mm_shuffle(3, 3, 3, 3));
    c->pshufhw(a0, a0, mm_shuffle(3, 3, 3, 3));
    c->paddw(a0, c0x0080.r());

    // composite over
    c->psubw(src0, dst0);       // src0 -= dst0
    c->pmullw(src0, a0);        // src0 *= a0
    c->psllw(dst0, 8);          // dst0 *= 256
    c->paddw(dst0, src0);       // dst0 += src0
    c->psrlw(dst0, 8);          // dst0 /= 256
  }

  virtual void doPixelUnpacked4(
    const XMMRegister& dst0, const XMMRegister& src0, 
    const XMMRegister& dst1, const XMMRegister& src1)
  {
    XMMRef _a0 = f->newVariable(VARIABLE_TYPE_XMM, 0);
    XMMRef _a1 = f->newVariable(VARIABLE_TYPE_XMM, 0);
    XMMRegister a0 = _a0.r();
    XMMRegister a1 = _a1.r();

    // expand a0, a1
    c->pshuflw(a0, src0, mm_shuffle(3, 3, 3, 3));
    c->pshuflw(a1, src1, mm_shuffle(3, 3, 3, 3));
    c->pshufhw(a0, a0, mm_shuffle(3, 3, 3, 3));
    c->pshufhw(a1, a1, mm_shuffle(3, 3, 3, 3));

    c->paddw(a0, c0x0080.r());
    c->paddw(a1, c0x0080.r());

    // composite over
    c->psubw(src0, dst0);       // src0 -= dst0
    c->psubw(src1, dst1);       // src1 -= dst1
    c->pmullw(src0, a0);        // src0 *= a0
    c->pmullw(src1, a1);        // src1 *= a1
    c->psllw(dst0, 8);          // dst0 *= 256
    c->psllw(dst1, 8);          // dst1 *= 256
    c->paddw(dst0, src0);       // dst0 += src0
    c->paddw(dst1, src1);       // dst1 += src1
    c->psrlw(dst0, 8);          // dst0 /= 256
    c->psrlw(dst1, 8);          // dst1 /= 256
  }
};
*/

// ============================================================================
// [BlitJit::Generator - Construction / Destruction]
// ============================================================================

Generator::Generator(Compiler* c) : 
  c(c),
  f(NULL),
  optimize(OptimizeX86),
  features(cpuInfo()->features),
  body(0)
{
  optimize = OptimizeSSE2;
}

Generator::~Generator()
{
}

// ============================================================================
// [BlitJit::Generator - fillSpan]
// ============================================================================

void Generator::fillSpan(const PixelFormat& pfDst, const PixelFormat& pfSrc, const Operation& op)
{
  initConstants();

  c->comment("BlitJit::Generator::fillSpan() - %s <- %s : %s", pfDst.name(), pfSrc.name(), op.name());

  f = c->newFunction(CConv, BuildFunction3<void*, UInt32, SysUInt>());
  f->setNaked(true);
  f->setAllocableEbp(true);

  // If operation is nop, do nothing - this should be better optimized in frontend
  if (op.id() == Operation::CompositeDest) { c->endFunction(); return; }

#if 0
  // Destination and source
  PtrRef dst = f->argument(0); 
  Int32Ref src = f->argument(1);
  SysIntRef cnt = f->argument(2);

  cnt.setPriority(0);
  cnt.alloc();

  dst.setPriority(0);
  dst.alloc();

  src.setPriority(0);
  src.alloc();

  // If source pixel format not contains alpha channel, add it
  if (!pfSrc.isAlpha())
  {
    c->or_(src.r(), pfDst.aMask32());
  }

  /*
  I don't know if this is useable
  if (op.dstAlphaUsed() && !op.srcAlphaUsed())
  {
    c->or_(src.r(), pfDst.aMask32());
  }
  */

  // --------------------------------------------------------------------------
  // [SSE2]
  // --------------------------------------------------------------------------

  if (1)
  {
    if (1)
    {
      // Labels
      Label* L_Align = c->newLabel();
      Label* L_Loop = c->newLabel();
      Label* L_End = c->newLabel();

      // We can process 4 pixels at a time
      XMMRef _dstpix0 = f->newVariable(VARIABLE_TYPE_XMM, 0);
      XMMRef _dstpix1 = f->newVariable(VARIABLE_TYPE_XMM, 0);

      // Source pixel is only one (we will expand it to xmm register later)
      XMMRef _srcpix0 = f->newVariable(VARIABLE_TYPE_XMM, 0);
      XMMRef _srctmp0 = f->newVariable(VARIABLE_TYPE_XMM, 0);
      XMMRef _srctmp1 = f->newVariable(VARIABLE_TYPE_XMM, 0);

      // Aplha value
      XMMRef _alpha = f->newVariable(VARIABLE_TYPE_XMM, 0);

      // Zero register 
      XMMRef _z = f->newVariable(VARIABLE_TYPE_XMM, 0);

      XMMRegister dstpix0 = _dstpix0.r();
      XMMRegister dstpix1 = _dstpix1.r();
      XMMRegister srcpix0 = _srcpix0.r();
      XMMRegister srctmp0 = _srctmp0.r();
      XMMRegister srctmp1 = _srctmp1.r();
      XMMRegister alpha = _alpha.r();
      XMMRegister z = _z.r();

      c->pxor(z, z);
      c->movd(srcpix0, src.r());
      c->punpcklbw(srcpix0, z);

      c->movaps(alpha, srcpix0);
      c->pshuflw(alpha, alpha, mm_shuffle(0, 0, 0, 0));
      c->pshufd(alpha, alpha, mm_shuffle(0, 0, 0, 0));
      //c->punpcklwd(alpha, alpha);         // r = [  ][  ][  ][  ][  ][  ][0A][0A]
      //c->punpckldq(alpha, alpha);         // r = [  ][  ][  ][  ][0A][0A][0A][0A]
      //c->punpcklqdq(alpha, alpha);        // r = [0A][0A][0A][0A][0A][0A][0A][0A]

      // ----------------------------------------------------------------------
      // [Align]
      // ----------------------------------------------------------------------

      // Skip aligning step if dst already aligned
      PtrRef t = f->newVariable(VARIABLE_TYPE_PTR, 0);
      t.alloc();

      c->bind(L_Align);

      c->mov(t.r(), dst.r());
      c->and_(t.r(), 15);
      c->jz(L_Loop);

      // Load
      if (op.dstPixelUsed() || 1)
      {
        switch (pfDst.id())
        {
          case PixelFormat::ARGB32:
          case PixelFormat::PRGB32:
          case PixelFormat::XRGB32:
            c->movd(dstpix0, dword_ptr(dst.r()));
            break;
        }
      }

      // Composite
      switch (op.id())
      {
        case Operation::CombineCopy:
          //break;
        case Operation::CombineBlend:
          c->punpcklbw(dstpix0, z);
          c->movaps(srctmp0, srcpix0);
          xmmLerp2(dstpix0, srctmp0, alpha);
          c->packuswb(dstpix0, dstpix0);
          break;
      }

      // Store
      switch (pfDst.id())
      {
        case PixelFormat::ARGB32:
        case PixelFormat::PRGB32:
        case PixelFormat::XRGB32:
          //c->movd(src.r(), dstpix0);
          //c->movnti(dword_ptr(dst.r()), src.r());
          c->movd(dword_ptr(dst.r()), dstpix0);
          break;
      }

      // End ?
      c->add(dst.r(), pfDst.depth() / 8);
      c->dec(cnt.r());
      c->jz(L_End);

      // Align more ?
      c->mov(t.r(), dst.r());
      c->and_(t.r(), 15);
      c->jnz(L_Align);

      // ----------------------------------------------------------------------
      // [Loop]
      // ----------------------------------------------------------------------

      c->bind(L_Loop);

      c->sub(dst.r(), 4);
      c->jo(L_End);

      // Load
      if (op.dstPixelUsed() || 1)
      {
        switch (pfDst.id())
        {
          case PixelFormat::ARGB32:
          case PixelFormat::PRGB32:
          case PixelFormat::XRGB32:
            c->movq(dstpix0, dword_ptr(dst.r(), 0));
            c->movq(dstpix1, dword_ptr(dst.r(), 8));
            break;
        }
      }

      // Composite
      switch (op.id())
      {
        case Operation::CombineCopy:
          //break;
        case Operation::CombineBlend:
          c->punpcklbw(dstpix0, z);
          c->punpcklbw(dstpix1, z);
          c->movaps(srctmp0, srcpix0);
          c->movaps(srctmp1, srcpix0);
          xmmLerp4(dstpix0, srctmp0, alpha, dstpix1, srctmp1, alpha);
          c->packuswb(dstpix0, dstpix0);
          c->packuswb(dstpix1, dstpix1);
          break;
      }

      // Store
      switch (pfDst.id())
      {
        case PixelFormat::ARGB32:
        case PixelFormat::PRGB32:
        case PixelFormat::XRGB32:
          c->movq(dword_ptr(dst.r(), 0), dstpix0);
          c->movq(dword_ptr(dst.r(), 8), dstpix1);
          break;
      }

      // ----------------------------------------------------------------------
      // [End]
      // ----------------------------------------------------------------------

      c->bind(L_End);
    }
  }

  // --------------------------------------------------------------------------
  // [X86]
  // --------------------------------------------------------------------------

  else
  {
    // We can optimize simple fills much more than compositing operations.
    // Simple fill should be unrolled loop for 32 or more bytes at a time.
    if (pfDst.depth() == 32 && op.id() == Operation::CombineCopy)
    {
      x86MemSet32(dst, src, cnt);
    }
    else
    {
      Int32Ref pixel = f->newVariable(VARIABLE_TYPE_UINT32, 0);
      pixel.alloc();

      Label* L_Loop = c->newLabel();
      c->bind(L_Loop);

      // Load
      if (op.dstPixelUsed())
      {
        switch (pfDst.id())
        {
          case PixelFormat::ARGB32:
          case PixelFormat::PRGB32:
          case PixelFormat::XRGB32:
            c->mov(pixel.x(), dword_ptr(src.r()));
            break;
        }
      }

      // Composite
      switch (op.id())
      {
        case Operation::CombineCopy:
          break;
      }

      // Store
      switch (pfDst.id())
      {
        case PixelFormat::ARGB32:
        case PixelFormat::PRGB32:
        case PixelFormat::XRGB32:
          c->mov(dword_ptr(dst.r()), pixel.r());
          break;
      }

      // End
      c->add(dst.r(), pfDst.depth() / 8);
      c->dec(cnt.r());
      c->jnz(L_Loop);
    }
  }
#endif
  c->endFunction();
}

void Generator::blitSpan(const PixelFormat& pfDst, const PixelFormat& pfSrc, const Operation& op)
{
  initConstants();

  c->comment("BlitJit::Generator::blitSpan() - %s <- %s : %s", pfDst.name(), pfSrc.name(), op.name());

  f = c->newFunction(CConv, BuildFunction3<void*, void*, SysUInt>());
  f->setNaked(true);
  f->setAllocableEbp(true);

  // If operation is nop, do nothing - this should be better optimized in frontend
  if (op.id() == Operation::CompositeDest) { c->endFunction(); return; }

  // Destination and source
  PtrRef dst = f->argument(0); 
  PtrRef src = f->argument(1);
  SysIntRef cnt = f->argument(2);

  cnt.setPriority(0);
  cnt.alloc();

  dst.setPriority(0);
  dst.alloc();

  src.setPriority(0);
  src.alloc();

  // [CombineCopy]
  if (op.id() == Operation::CompositeSrc)
  {
    if (pfDst.depth() == 32 && pfSrc.depth() == 32)
    {
      _MemCpy32(dst, src, cnt);
    }
  }
  // [Combine...]
  else
  {
    if (optimize == OptimizeSSE2)
    {
      GeneratorOpComposite32_SSE2 c_op(this, c, f, op.id());
      c_op.init();
      _Composite32_SSE2(dst, src, cnt, c_op);
      c_op.free();
    }
  }

  c->endFunction();
}

void Generator::blitRect(const PixelFormat& pfDst, const PixelFormat& pfSrc, const Operation& op)
{
  initConstants();

  c->comment("BlitJit::Generator::blitRect() - %s <- %s : %s", pfDst.name(), pfSrc.name(), op.name());

  f = c->newFunction(CConv, BuildFunction6<void*, void*, SysInt, SysInt, SysUInt, SysUInt>());
  f->setNaked(true);
  f->setAllocableEbp(true);

  // If operation is nop, do nothing - this should be better optimized in frontend
  if (op.id() == Operation::CompositeDest) { c->endFunction(); return; }

  // Destination and source
  PtrRef dst = f->argument(0);
  PtrRef src = f->argument(1);
  SysIntRef dstStride = f->argument(2);
  SysIntRef srcStride = f->argument(3);
  SysIntRef width = f->argument(4);
  SysIntRef height = f->argument(5);

  SysIntRef cnt(f->newVariable(VARIABLE_TYPE_SYSINT));

  // Adjust dstStride and srcStride
  SysIntRef t(f->newVariable(VARIABLE_TYPE_SYSINT));

  c->mov(t.r(), width);
  c->shl(t.r(), 2);

  c->sub(dstStride, t.r());
  c->sub(srcStride, t.r());

  t.destroy();

  cnt.setPriority(0);
  cnt.alloc();

  dst.setPriority(0);
  dst.alloc();

  src.setPriority(0);
  src.alloc();

  // [CombineCopy]
  if (op.id() == Operation::CompositeSrc)
  {
    if (pfDst.depth() == 32 && pfSrc.depth() == 32)
    {
      Label* L_Loop = c->newLabel();
      c->bind(L_Loop);
      c->mov(cnt.r(), width);

      _MemCpy32(dst, src, cnt);

      c->add(dst.r(), dstStride);
      c->add(src.r(), srcStride);
      c->dec(height);
      c->jnz(L_Loop);
    }
  }
  // [Combine...]
  else
  {
    if (optimize == OptimizeSSE2)
    {
      GeneratorOpComposite32_SSE2 c_op(this, c, f, op.id());
      c_op.init();

      Label* L_Loop = c->newLabel();
      c->bind(L_Loop);
      c->mov(cnt.r(), width);

      _Composite32_SSE2(dst, src, cnt, c_op);

      c->add(dst.r(), dstStride);
      c->add(src.r(), srcStride);
      c->dec(height);
      c->jnz(L_Loop);

      c_op.free();
    }
  }

  c->endFunction();
}

void Generator::_MemCpy32(PtrRef& dst, PtrRef& src, SysIntRef& cnt)
{
  // --------------------------------------------------------------------------
  // [MemCopy32 - X86]
  // --------------------------------------------------------------------------
  if (optimize == OptimizeX86)
  {
    Int32Ref pix = f->newVariable(VARIABLE_TYPE_INT32, 0);
    pix.alloc();

    Label* L_Loop = c->newLabel();
    c->bind(L_Loop);

    c->mov(pix.r(), dword_ptr(src.r()));
    c->add(src.r(), 4);
    c->mov(dword_ptr(dst.r()), pix.r());
    c->add(dst.r(), 4);

    c->dec(cnt.r());
    c->jnz(L_Loop);
  }

  // --------------------------------------------------------------------------
  // [MemCopy32 - MMX]
  // --------------------------------------------------------------------------
  else if (optimize == OptimizeMMX)
  {
    SysIntRef t = f->newVariable(VARIABLE_TYPE_SYSINT, 0);

    MMRef _v0 = f->newVariable(VARIABLE_TYPE_MM, 0);
    MMRef _v1 = f->newVariable(VARIABLE_TYPE_MM, 0);
    MMRef _v2 = f->newVariable(VARIABLE_TYPE_MM, 0);
    MMRef _v3 = f->newVariable(VARIABLE_TYPE_MM, 0);

    MMRegister v0 = _v0.r();
    MMRegister v1 = _v1.r();
    MMRegister v2 = _v2.r();
    MMRegister v3 = _v3.r();

    Label* L_Aligned = c->newLabel();
    Label* L_Loop    = c->newLabel();
    Label* L_Tail    = c->newLabel();
    Label* L_TailLoop= c->newLabel();
    Label* L_TailEnd = c->newLabel();
    Label* L_Exit    = c->newLabel();

    Int32 mainLoopSize = 64;
    Int32 i;

    // ------------------------------------------------------------------------
    // [Align]
    // ------------------------------------------------------------------------

    c->mov(t.r(), dst.r());
    c->and_(t.r(), 7);
    c->jz(L_Aligned, HINT_TAKEN);

    c->dec(cnt.r());

    c->mov(t.r32(), ptr(src.r()));
    c->add(src.r(), 4);
    c->mov(ptr(dst.r()), t.r32());
    c->add(dst.r(), 4);

    c->bind(L_Aligned);

    // ------------------------------------------------------------------------
    // [Loop]
    // ------------------------------------------------------------------------

    c->sub(cnt.r(), mainLoopSize / 4);
    c->jc(L_Tail, HINT_NOT_TAKEN);

    c->align(8);
    c->bind(L_Loop);

    //c->prefetch(ptr(src.r(), mainLoopSize), PREFETCH_T0);
    //c->prefetch(ptr(dst.r(), mainLoopSize), PREFETCH_T0);

    for (i = 0; i < mainLoopSize; i += 32)
    {
      c->movq(v0, ptr(src.r(), i +  0));
      c->movq(v1, ptr(src.r(), i +  8));
      c->movq(v2, ptr(src.r(), i + 16));
      c->movq(v3, ptr(src.r(), i + 24));
      c->movq(ptr(dst.r(), i +  0), v0);
      c->movq(ptr(dst.r(), i +  8), v1);
      c->movq(ptr(dst.r(), i + 16), v2);
      c->movq(ptr(dst.r(), i + 24), v3);
    }

    c->add(src.r(), mainLoopSize);
    c->add(dst.r(), mainLoopSize);

    c->sub(cnt.r(), mainLoopSize / 4);
    c->jnc(L_Loop);

    // ------------------------------------------------------------------------
    // [Tail]
    // ------------------------------------------------------------------------

    c->add(cnt.r(), mainLoopSize / 4);
    c->jz(L_Exit);

    c->bind(L_Tail);
    c->mov(t.r(), 2);
    c->cmp(t.r(), cnt.r());
    c->jnle(L_TailEnd);

    c->bind(L_TailLoop);
    c->movq(v0, ptr(src.r(), t.r(), 4, -8));
    c->movq(ptr(dst.r(), t.r(), 4, -8), v0);

    c->add(t.r(), 2);
    c->cmp(t.r(), cnt.r());
    c->jle(L_TailLoop);

    c->bind(L_TailEnd);
    c->lea(src.r(), ptr(src.r(), cnt.r(), 2));
    c->lea(dst.r(), ptr(dst.r(), cnt.r(), 2));

    c->and_(cnt.r(), 1);
    c->jz(L_Exit);

    c->mov(t.r32(), dword_ptr(src.r(), -4));
    c->mov(dword_ptr(dst.r(), -4), t.r32());

    c->bind(L_Exit);
  }

  // --------------------------------------------------------------------------
  // [MemCopy32 - SSE2]
  // --------------------------------------------------------------------------
  else if (optimize == OptimizeSSE2)
  {
    SysIntRef t = f->newVariable(VARIABLE_TYPE_SYSINT, 0);

    XMMRef _v0 = f->newVariable(VARIABLE_TYPE_XMM, 0);
    XMMRef _v1 = f->newVariable(VARIABLE_TYPE_XMM, 0);
    XMMRef _v2 = f->newVariable(VARIABLE_TYPE_XMM, 0);
    XMMRef _v3 = f->newVariable(VARIABLE_TYPE_XMM, 0);

    XMMRegister v0 = _v0.r();
    XMMRegister v1 = _v1.r();
    XMMRegister v2 = _v2.r();
    XMMRegister v3 = _v3.r();

    Label* L_Align   = c->newLabel();
    Label* L_Aligned = c->newLabel();
    Label* L_Loop    = c->newLabel();
    Label* L_Tail    = c->newLabel();
    Label* L_TailLoop= c->newLabel();
    Label* L_TailEnd = c->newLabel();
    Label* L_Exit    = c->newLabel();

    Int32 mainLoopSize = 64;
    Int32 i;

    // ------------------------------------------------------------------------
    // [Align]
    // ------------------------------------------------------------------------

    // For smaller size, we will use tail loop
    c->cmp(cnt.r(), 8);
    c->jl(L_Tail);

    c->xor_(t.r(), t.r());
    c->sub(t.r(), dst.r());
    c->and_(t.r(), 15);
    c->jz(L_Aligned);

    c->shr(t.r(), 2);
    c->sub(cnt.r(), t.r());

    c->bind(L_Align);
    c->movd(v0, ptr(src.r()));
    c->movd(ptr(dst.r()), v0);
    c->add(src.r(), 4);
    c->add(dst.r(), 4);
    c->dec(t.r());
    c->jnz(L_Align, HINT_TAKEN);

    // This shouldn't happen, but we must be sure
    c->mov(t.r(), dst.r());
    c->and_(t.r(), 3);
    c->jnz(L_Tail);

    c->bind(L_Aligned);

    c->sub(cnt.r(), mainLoopSize / 4);
    c->jc(L_Tail, HINT_NOT_TAKEN);

    // ------------------------------------------------------------------------
    // [Loop]
    // ------------------------------------------------------------------------

    c->align(16);
    c->bind(L_Loop);

    c->prefetch(ptr(src.r(), mainLoopSize), PREFETCH_T0);
    //c->prefetch(ptr(dst.r(), mainLoopSize), PREFETCH_T0);

    for (i = 0; i < mainLoopSize; i += 64)
    {
      c->movdqu(v0, ptr(src.r(), i +  0));
      c->movdqu(v1, ptr(src.r(), i + 16));
      c->movdqu(v2, ptr(src.r(), i + 32));
      c->movdqu(v3, ptr(src.r(), i + 48));
      c->movdqu(ptr(dst.r(), i +  0), v0);
      c->movdqu(ptr(dst.r(), i + 16), v1);
      c->movdqu(ptr(dst.r(), i + 32), v2);
      c->movdqu(ptr(dst.r(), i + 48), v3);
    }

    c->add(src.r(), mainLoopSize);
    c->add(dst.r(), mainLoopSize);

    c->sub(cnt.r(), mainLoopSize / 4);
    c->jnc(L_Loop);

    // ------------------------------------------------------------------------
    // [Tail]
    // ------------------------------------------------------------------------

    c->add(cnt.r(), mainLoopSize / 4);
    c->jz(L_Exit);

    c->bind(L_Tail);
    c->mov(t.r(), 2);
    c->cmp(t.r(), cnt.r());
    c->jnle(L_TailEnd);

    c->bind(L_TailLoop);
    c->movq(v0, ptr(src.r(), t.r(), 2, -8));
    c->movq(ptr(dst.r(), t.r(), 2, -8), v0);

    c->add(t.r(), 2);
    c->cmp(t.r(), cnt.r());
    c->jle(L_TailLoop);

    c->bind(L_TailEnd);
    c->lea(src.r(), ptr(src.r(), cnt.r(), 2));
    c->lea(dst.r(), ptr(dst.r(), cnt.r(), 2));

    c->and_(cnt.r(), 1);
    c->jz(L_Exit);

    c->mov(t.r32(), dword_ptr(src.r(), -4));
    c->mov(dword_ptr(dst.r(), -4), t.r32());

    c->bind(L_Exit);
  }
}

void Generator::_Composite32_SSE2(
  PtrRef& dst, PtrRef& src, SysIntRef& cnt,
  GeneratorOpComposite32_SSE2& c_op)
{
  SysIntRef t = f->newVariable(VARIABLE_TYPE_SYSINT, 0);

  XMMRef _dstpix0 = f->newVariable(VARIABLE_TYPE_XMM, 0);
  XMMRef _dstpix1 = f->newVariable(VARIABLE_TYPE_XMM, 0);
  XMMRef _srcpix0 = f->newVariable(VARIABLE_TYPE_XMM, 0);
  XMMRef _srcpix1 = f->newVariable(VARIABLE_TYPE_XMM, 0);

  XMMRegister srcpix0 = _srcpix0.r();
  XMMRegister srcpix1 = _srcpix1.r();
  XMMRegister dstpix0 = _dstpix0.r();
  XMMRegister dstpix1 = _dstpix1.r();

#if defined(UNROLL_8)
  XMMRef _dstpix2 = f->newVariable(VARIABLE_TYPE_XMM, 0);
  XMMRef _dstpix3 = f->newVariable(VARIABLE_TYPE_XMM, 0);
  XMMRef _srcpix2 = f->newVariable(VARIABLE_TYPE_XMM, 0);
  XMMRef _srcpix3 = f->newVariable(VARIABLE_TYPE_XMM, 0);

  XMMRegister srcpix2 = _srcpix2.r();
  XMMRegister srcpix3 = _srcpix3.r();
  XMMRegister dstpix2 = _dstpix2.r();
  XMMRegister dstpix3 = _dstpix3.r();
#endif // UNROLL_8

  Label* L_Align   = c->newLabel();
  Label* L_Aligned = c->newLabel();
  Label* L_Loop    = c->newLabel();
  Label* L_Tail    = c->newLabel();
  Label* L_Exit    = c->newLabel();

  Int32 mainLoopSize = 16;

#if defined(UNROLL_8)
  mainLoopSize += 16;
#endif // UNROLL_8

  Int32 i;
  UInt32 unroll = 1;

  unroll &= c_op.unroll;

  if (unroll) t.alloc();

  // ------------------------------------------------------------------------
  // [Align]
  // ------------------------------------------------------------------------

  if (unroll)
  {
    // For small size, we will use tail loop
    c->cmp(cnt.r(), 4);
    c->jl(L_Tail);

    c->xor_(t.r(), t.r());
    c->sub(t.r(), dst.r());
    c->and_(t.r(), 15);
    c->jz(L_Aligned);

    c->shr(t.r(), 2);
    c->sub(cnt.r(), t.r());

    c->bind(L_Align);
    c->movd(srcpix0, ptr(src.r()));
    c->movd(dstpix0, ptr(dst.r()));
    c_op.doPixelRaw(dstpix0, srcpix0, false);
    c->movd(ptr(dst.r()), dstpix0);
    c->add(src.r(), 4);
    c->add(dst.r(), 4);
    c->dec(t.r());
    c->jnz(L_Align);

    // This shouldn't happen, but we must be sure
    c->mov(t.r(), dst.r());
    c->and_(t.r(), 3);
    c->jnz(L_Tail);

    c->bind(L_Aligned);
  }

  // ------------------------------------------------------------------------
  // [Loop]
  // ------------------------------------------------------------------------

  if (unroll)
  {
    c->sub(cnt.r(), mainLoopSize / 4);
    c->jc(L_Tail);

    // Loop - Src is Unaligned
    c->align(16);
    c->bind(L_Loop);

    //c->prefetch(ptr(src.r(), mainLoopSize), PREFETCH_T0);
    //c->prefetch(ptr(dst.r(), mainLoopSize), PREFETCH_T0);

#if defined(UNROLL_8)
    for (i = 0; i < mainLoopSize; i += 32)
    {
      c->movq(srcpix0, ptr(src.r(), i +  0));
      c->movq(srcpix1, ptr(src.r(), i +  8));
      c->movq(srcpix2, ptr(src.r(), i + 16));
      c->movq(srcpix3, ptr(src.r(), i + 24));

      c->movdqa(dstpix0, ptr(dst.r(), i +  0));
      c->movdqa(dstpix2, ptr(dst.r(), i + 16));

      c_op.doPixelRaw_8(
        dstpix0, srcpix0, dstpix1, srcpix1,
        dstpix2, srcpix2, dstpix3, srcpix3,
        GeneratorOpComposite32_SSE2::Raw4UnpackFromDst0 |
        GeneratorOpComposite32_SSE2::Raw4UnpackFromDst2 |
        GeneratorOpComposite32_SSE2::Raw4PackToDst0 |
        GeneratorOpComposite32_SSE2::Raw4PackToDst2);

      c->movdqa(ptr(dst.r(), i +  0), dstpix0);
      c->movdqa(ptr(dst.r(), i + 16), dstpix2);
    }
#else
    for (i = 0; i < mainLoopSize; i += 16)
    {
      //c->movdqu(srcpix0, ptr(src.r(), i + 0));
      c->movq(srcpix0, ptr(src.r(), i + 0));
      c->movq(srcpix1, ptr(src.r(), i + 8));

      // We are using movdqa instead of two movq instructions, it
      // should be faster than this:
      //   c->movq(dstpix0, ptr(dst.r(), i + 0));
      //   c->movq(dstpix1, ptr(dst.r(), i + 8));
      c->movdqa(dstpix0, ptr(dst.r(), i + 0));

      // If we used movdqa and destination is only in dstpix0, we must set
      // Raw4UnpackFromDst0 flag. We also want to pack resulting pixels to
      // dstpix0 so we also set Raw4PackToDst0.
      c_op.doPixelRaw_4(dstpix0, srcpix0, dstpix1, srcpix1,
        GeneratorOpComposite32_SSE2::Raw4UnpackFromDst0 |
        GeneratorOpComposite32_SSE2::Raw4PackToDst0);

      // movdqa also instead of two movq:
      //   c->movq(ptr(dst.r(), i + 0), dstpix0);
      //   c->movq(ptr(dst.r(), i + 8), dstpix1);
      c->movdqa(ptr(dst.r(), i + 0), dstpix0);
    }
#endif // UNROLL_8
    c->add(src.r(), mainLoopSize);
    c->add(dst.r(), mainLoopSize);

    c->sub(cnt.r(), mainLoopSize / 4);
    c->jnc(L_Loop);

    c->add(cnt.r(), mainLoopSize / 4);
    c->jz(L_Exit);
  }

  // ------------------------------------------------------------------------
  // [Tail]
  // ------------------------------------------------------------------------

  if (unroll)
  {
    Label* L_TailSkip = c->newLabel();

    c->sub(cnt.r(), 2);
    c->jc(L_TailSkip);

    c->align(8);
    c->bind(L_Tail);

    c->movq(srcpix0, ptr(src.r()));
    c->movq(dstpix0, ptr(dst.r()));
    c_op.doPixelRaw(dstpix0, srcpix0, true);
    c->movq(ptr(dst.r()), dstpix0);

    c->add(src.r(), 8);
    c->add(dst.r(), 8);

    c->sub(cnt.r(), 2);
    c->jnc(L_Tail);

    c->bind(L_TailSkip);
    c->add(cnt.r(), 2);
    c->jz(L_Exit);

    c->movd(srcpix0, ptr(src.r()));
    c->movd(dstpix0, ptr(dst.r()));
    c_op.doPixelRaw(dstpix0, srcpix0, false);
    c->movd(ptr(dst.r()), dstpix0);

    c->add(src.r(), 4);
    c->add(dst.r(), 4);
  }
  else
  {
    c->align(8);
    c->bind(L_Tail);

    c->movd(srcpix0, ptr(src.r()));
    c->movd(dstpix0, ptr(dst.r()));
    c_op.doPixelRaw(dstpix0, srcpix0, false);
    c->movd(ptr(dst.r()), dstpix0);

    c->add(src.r(), 4);
    c->add(dst.r(), 4);
    c->dec(cnt.r());
    c->jnz(L_Tail);
  }

  c->bind(L_Exit);
}

// ============================================================================
// [BlitJit::Generator - Helpers]
// ============================================================================

void Generator::x86MemSet32(PtrRef& dst, Int32Ref& src, SysIntRef& cnt)
{
  SysIntRef tmp = f->newVariable(VARIABLE_TYPE_SYSINT, 0);
  tmp.alloc();

  Label* L1 = c->newLabel();
  Label* L2 = c->newLabel();

  c->cmp(cnt.r(), 8);
  c->jl(L2);

  c->mov(tmp.r(), cnt.r());
  c->shr(cnt.r(), 3);
  c->and_(tmp.r(), 7);

  // Unrolled loop, we are using align 4 here, because this path will be 
  // generated only for older CPUs.
  c->align(4);
  c->bind(L1);

  c->mov(dword_ptr(dst.r(),  0), src.r());
  c->mov(dword_ptr(dst.r(),  4), src.r());
  c->mov(dword_ptr(dst.r(),  8), src.r());
  c->mov(dword_ptr(dst.r(), 12), src.r());
  c->mov(dword_ptr(dst.r(), 16), src.r());
  c->mov(dword_ptr(dst.r(), 20), src.r());
  c->mov(dword_ptr(dst.r(), 24), src.r());
  c->mov(dword_ptr(dst.r(), 28), src.r());

  c->add(dst.r(), 32);
  c->dec(cnt.r());
  c->jnz(L1);

  c->mov(cnt.r(), tmp.r());
  c->jz(c->currentFunction()->exitLabel());

  // Tail
  c->align(4);
  c->bind(L2);

  c->mov(dword_ptr(dst.r()), src.r());

  c->add(dst.r(), 4);
  c->dec(cnt.r());
  c->jnz(L2);
}

void Generator::xmmExpandAlpha_0000AAAA(const XMMRegister& r)
{
  // r = [  ][  ][  ][  ][  ][  ][  ][0A]

  if ((features & CpuInfo::Feature_SSE) != 0)
  {
    // r = [  ][  ][  ][  ][0A][0A][0A][0A]
    c->pshufd(r, r, mm_shuffle(0, 0, 0, 0));
  }
  else
  {
    // r = [  ][  ][  ][  ][  ][  ][0A][0A]
    c->punpcklwd(r, r);
    // r = [  ][  ][  ][  ][0A][0A][0A][0A]
    c->punpckldq(r, r);
  }
}

void Generator::xmmExpandAlpha_AAAAAAAA(const XMMRegister& r)
{
  // r = [  ][  ][  ][  ][0A][0A][0A][0A]
  xmmExpandAlpha_0000AAAA(r);
  // r = [0A][0A][0A][0A][0A][0A][0A][0A]
  c->punpcklqdq(r, r);
}

void Generator::xmmExpandAlpha_00001AAA(const XMMRegister& r, const XMMRegister& c0)
{
/*
  // (r)                      // r = [  ][  ][  ][  ][  ][  ][  ][0A]
  c->punpcklwd(r, r);         // r = [  ][  ][  ][  ][  ][  ][0A][0A]
  c->punpckldq(r, r);         // r = [  ][  ][  ][  ][0A][0A][0A][0A]
*/
}

void Generator::xmmExpandAlpha_1AAA1AAA(const XMMRegister& r, const XMMRegister& c0)
{
}

void Generator::xmmLerp2(
  const XMMRegister& dst0, const XMMRegister& src0, const XMMRegister& alpha0)
{
  c->psubw(src0, dst0);       // src0 -= dst0
  c->pmullw(src0, alpha0);    // src0 *= alpha0
  c->psllw(dst0, 8);          // dst0 *= 256
  c->paddw(dst0, src0);       // dst0 += src0
  c->psrlw(dst0, 8);          // dst0 /= 256
}

void Generator::xmmLerp4(
  const XMMRegister& dst0, const XMMRegister& src0, const XMMRegister& alpha0,
  const XMMRegister& dst1, const XMMRegister& src1, const XMMRegister& alpha1)
{
  c->psubw(src0, dst0);       // src0 -= dst0
  c->psubw(src1, dst1);       // src1 -= dst1
  c->pmullw(src0, alpha0);    // src0 *= alpha0
  c->pmullw(src1, alpha1);    // src1 *= alpha1
  c->psllw(dst0, 8);          // dst0 *= 256
  c->psllw(dst1, 8);          // dst1 *= 256
  c->paddw(dst0, src0);       // dst0 += src0
  c->paddw(dst1, src1);       // dst1 += src1
  c->psrlw(dst0, 8);          // dst0 /= 256
  c->psrlw(dst1, 8);          // dst1 /= 256
}

void Generator::initRConst()
{
  if (body & BodyRConst) return;

  rconst.use(f->newVariable(VARIABLE_TYPE_PTR, 0));
  rconst.alloc();
  c->mov(rconst.r(), (SysInt)constants);

  body |= BodyRConst;
}

// ============================================================================
// [BlitJit::Generator - Constants]
// ============================================================================

Generator::Constants* Generator::constants = NULL;

void Generator::initConstants()
{
  static UInt8 constantsStorage[sizeof(Constants) + 16];

  if (constants == NULL)
  {
    SysInt i;

    // Align to 16 bytes (default SSE alignment)
    Constants* c = (Constants*)(void*)(((SysUInt)constantsStorage + 15) & ~15);

    //c->Cx00000000000001000000000000000100.set_uw(0x0000, 0x0000, 0x0000, 0x0100, 0x0000, 0x0000, 0x0000, 0x0100);
    c->Cx00800080008000800080008000800080.set_uw(0x0080, 0x0080, 0x0080, 0x0080, 0x0080, 0x0080, 0x0080, 0x0080);
    c->Cx00FF00FF00FF00FF00FF00FF00FF00FF.set_uw(0x00FF, 0x00FF, 0x00FF, 0x00FF, 0x00FF, 0x00FF, 0x00FF, 0x00FF);
    //c->Cx000000000000FFFF000000000000FFFF.set_uw(0x0000, 0x0000, 0x0000, 0xFFFF, 0x0000, 0x0000, 0x0000, 0xFFFF);
    //c->CxFFFFFFFFFFFF0100FFFFFFFFFFFF0100.set_uw(0xFFFF, 0xFFFF, 0xFFFF, 0x0100, 0xFFFF, 0xFFFF, 0xFFFF, 0x0100);

    for (i = 0; i < 256; i++)
    {
      UInt16 x = i > 0 ? (int)((256.0 * 255.0) / (float)i + 0.5) : 0;
      c->CxDemultiply[i].set_uw(i, i, i, i, i, i, i, i);
    }

    constants = c;
  }
}

} // BlitJit namespace
