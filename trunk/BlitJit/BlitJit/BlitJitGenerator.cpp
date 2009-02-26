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

using namespace AsmJit;

namespace BlitJit {

#define RCONST_DISP(__cname__) (Int32)( (UInt8 *)&Generator::constants->__cname__ - (UInt8 *)Generator::constants )

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

  cnt.setPreferredRegister(REG_NCX);
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

// ============================================================================
// [BlitJit::Generator - blitSpan]
// ============================================================================

struct CompositeOp32
{
  CompositeOp32(Generator* g, Compiler* c, Function* f) : g(g), c(c), f(f) {}
  virtual ~CompositeOp32() {}

  enum RawFlags
  {
    Raw4UnpackFromDst0 = (1 << 0),
    Raw4UnpackFromSrc0 = (1 << 1),
    Raw4PackToDst0 = (1 << 2)
  };

  virtual void init() {};
  virtual void free() {};

  virtual void doRawPixel1(
    const XMMRegister& dst0, const XMMRegister& src0) {}

  virtual void doRawPixel2(
    const XMMRegister& dst0, const XMMRegister& src0) {}

  virtual void doRawPixel4(
    const XMMRegister& dst0, const XMMRegister& src0,
    const XMMRegister& dst1, const XMMRegister& src1, UInt32 flags) {}

  virtual void doUnpackedPixel1(
    const XMMRegister& dst0, const XMMRegister& src0) {}

  virtual void doUnpackedPixel2(
    const XMMRegister& dst0, const XMMRegister& src0) {}

  virtual void doUnpackedPixel4(
    const XMMRegister& dst0, const XMMRegister& src0, 
    const XMMRegister& dst1, const XMMRegister& src1) {}

  Generator* g;
  Compiler* c;
  Function* f;
};

struct CompositeOp32_Base : public CompositeOp32
{
  CompositeOp32_Base(Generator* g, Compiler* c, Function* f) : 
    CompositeOp32(g, c, f) {}
  virtual ~CompositeOp32_Base() {}

  virtual void init()
  {
    g->initRConst();

    z.use(f->newVariable(VARIABLE_TYPE_XMM, 0));
    z.alloc();

    c0x0080.use(f->newVariable(VARIABLE_TYPE_XMM, 0));
    c0x0080.alloc();

    c->pxor(z.r(), z.r());
    c->movq(c0x0080.r(), ptr(g->rconst.r(), 
      RCONST_DISP(Cx00800080008000800080008000800080)));
  }

  virtual void free()
  {
    z.unuse();
  }

  virtual void doRawPixel1(
    const XMMRegister& dst0, const XMMRegister& src0)
  {
    c->punpcklbw(src0, z.r());
    c->punpcklbw(dst0, z.r());
    doUnpackedPixel1(dst0, src0);
    c->packuswb(dst0, dst0);
  }

  virtual void doRawPixel2(
    const XMMRegister& dst0, const XMMRegister& src0)
  {
    c->punpcklbw(src0, z.r());
    c->punpcklbw(dst0, z.r());
    doUnpackedPixel2(dst0, src0);
    c->packuswb(dst0, dst0);
  }

  virtual void doRawPixel4(
    const XMMRegister& dst0, const XMMRegister& src0,
    const XMMRegister& dst1, const XMMRegister& src1, UInt32 flags)
  {
    if (flags & Raw4UnpackFromSrc0) c->movdqa(src1, src0);
    if (flags & Raw4UnpackFromDst0) c->movdqa(dst1, dst0);

    if (flags & Raw4UnpackFromSrc0)
    {
      c->punpcklbw(src0, z.r());
      c->punpckhbw(src1, z.r());
    }
    else
    {
      c->punpcklbw(src0, z.r());
      c->punpcklbw(src1, z.r());
    }

    if (flags & Raw4UnpackFromDst0)
    {
      c->punpcklbw(dst0, z.r());
      c->punpckhbw(dst1, z.r());
    }
    else
    {
      c->punpcklbw(dst0, z.r());
      c->punpcklbw(dst1, z.r());
    }

    doUnpackedPixel4(dst0, src0, dst1, src1);

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

  //! @brief Extract alpha channel.
  //! @param dst0 Destination XMM register (can be same as @a src).
  //! @param src0 Source XMM register.
  //! @param packed Whether extract alpha for packed pixels (two pixels).
  //! @param alphaPos Alpha position.
  //! @param negate Whether to negate extracted alpha value.
  void doExtractAlpha(const XMMRegister& dst0, const XMMRegister& src0, UInt8 packed, UInt8 alphaPos, UInt8 negate = false)
  {
    c->pshuflw(dst0, src0, mm_shuffle(alphaPos, alphaPos, alphaPos, alphaPos));

    if (packed)
    {
      c->pshufhw(dst0, dst0, mm_shuffle(alphaPos, alphaPos, alphaPos, alphaPos));
    }

    if (negate)
    {
      c->pxor(dst0, ptr(g->rconst.r(),
        RCONST_DISP(Cx00FF00FF00FF00FF00FF00FF00FF00FF)));
    }
  }

  void doPackedMultiply2(
    const XMMRegister& a0, const XMMRegister& b0, const XMMRegister& t0)
  {
    XMMRegister r = c0x0080.r();

    c->pmullw(a0, b0);          // a0 *= b0
    c->paddusw(a0, r);          // a0 += 80
    c->movdqa(t0, a0);          // t0  = a0
    c->psrlw(a0, 8);            // a0 /= 256
    c->paddusw(a0, t0);         // a0 += t0
    c->psrlw(a0, 8);            // a0 /= 256
  }

  void doPackedMultiply4(
    const XMMRegister& a0, const XMMRegister& b0, const XMMRegister& t0,
    const XMMRegister& a1, const XMMRegister& b1, const XMMRegister& t1)
  {
    XMMRegister r = c0x0080.r();

    c->pmullw(a0, b0);          // a0 *= b0
    c->pmullw(a1, b1);          // a1 *= b1
    c->paddusw(a0, r);          // a0 += 80
    c->paddusw(a1, r);          // a1 += 80
    c->movdqa(t0, a0);          // t0  = a0
    c->movdqa(t1, a1);          // t1  = a1
    c->psrlw(a0, 8);            // a0 /= 256
    c->psrlw(a1, 8);            // a1 /= 256
    c->paddusw(a0, t0);         // a0 += t0
    c->paddusw(a1, t1);         // a1 += t1
    c->psrlw(a0, 8);            // a0 /= 256
    c->psrlw(a1, 8);            // a1 /= 256
  }

  void doPackedMultiplyAdd2(
    const XMMRegister& a0, const XMMRegister& b0,
    const XMMRegister& c0, const XMMRegister& d0,
    const XMMRegister& t0)
  {
    XMMRegister r = c0x0080.r();

    c->pmullw(a0, b0);          // a0 *= b0
    c->pmullw(c0, d0);          // c0 *= d0
    c->paddusw(a0, r);          // a0 += 80
    c->paddusw(a0, c0);         // a0 += c0

    c->movdqa(t0, a0);          // t0  = a0
    c->psrlw(a0, 8);            // a0 /= 256
    c->paddusw(a0, t0);         // a0 += t0
    c->psrlw(a0, 8);            // a0 /= 256

/*
    a0 = _mm_mullo_pi16 (a0, b0);
    c0 = _mm_mullo_pi16 (c0, d0);
    a0 = _mm_adds_pu16 (a0, MC(4x0080));
    a0 = _mm_adds_pu16 (a0, c0);
    a0 = _mm_adds_pu16 (a0, _mm_srli_pi16 (a0, 8));
    a0 = _mm_srli_pi16 (a0, 8);
*/
  }

  XMMRef z;
  XMMRef c0x0080;
};
/*
struct CompositeOp32_Over : public CompositeOp32_Base
{
  CompositeOp32_Over(Generator* g, Compiler* c, Function* f) : 
    CompositeOp32_Base(g, c, f) {}
  virtual ~CompositeOp32_Over() {}

  virtual void doUnpackedPixel1(
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

  virtual void doUnpackedPixel2(
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

  virtual void doUnpackedPixel4(
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

struct CompositeOp32_Over : public CompositeOp32_Base
{
  CompositeOp32_Over(Generator* g, Compiler* c, Function* f, UInt32 op) : 
    CompositeOp32_Base(g, c, f), op(op) {}
  virtual ~CompositeOp32_Over() {}

  virtual void doUnpackedPixel1(
    const XMMRegister& dst0, const XMMRegister& src0)
  {
    XMMRef _a0 = f->newVariable(VARIABLE_TYPE_XMM, 0);
    XMMRef _t0 = f->newVariable(VARIABLE_TYPE_XMM, 0);

    XMMRegister a0 = _a0.r();
    XMMRegister t0 = _t0.r();

    switch (op)
    {
      case Operation::CompositeSrc:
        // copy operation (optimized in frontends and also by Generator itself)
        c->movdqa(dst0, src0);
        break;
      case Operation::CompositeDest:
        // no operation (optimized in frontends and also by Generator itself)
        break;
      //case Operation::CombineCopy:
      //case Operation::CombineBlend:
      case Operation::CompositeOver:
        doExtractAlpha(a0, src0, 0, 3, true);
        doPackedMultiply2(dst0, a0, t0);
        c->paddusb(dst0, src0);
        break;
      case Operation::CompositeOverReverse:
        doExtractAlpha(a0, dst0, 0, 3, true);
        doPackedMultiply2(src0, a0, t0);
        c->paddusb(dst0, src0);
        break;
      case Operation::CompositeIn:
        doExtractAlpha(a0, dst0, 0, 3, false);
        doPackedMultiply2(src0, a0, t0);
        c->movdqa(dst0, src0);
        break;
      case Operation::CompositeInReverse:
        doExtractAlpha(a0, src0, 0, 3, false);
        doPackedMultiply2(dst0, a0, t0);
        break;
      case Operation::CompositeOut:
        doExtractAlpha(a0, dst0, 0, 3, true);
        doPackedMultiply2(src0, a0, t0);
        c->movdqa(dst0, src0);
        break;
      case Operation::CompositeOutReverse:
        doExtractAlpha(a0, src0, 0, 3, true);
        doPackedMultiply2(dst0, a0, t0);
        break;
      case Operation::CompositeAtop:
        doExtractAlpha(a0, src0, 0, 3, true);
        doExtractAlpha(t0, dst0, 0, 3, false);
        doPackedMultiplyAdd2(src0, t0, dst0, a0, t0);
        c->movdqa(dst0, src0);
        break;
      case Operation::CompositeAtopReverse:
        doExtractAlpha(a0, src0, 0, 3, false);
        doExtractAlpha(t0, dst0, 0, 3, true);
        doPackedMultiplyAdd2(src0, t0, dst0, a0, t0);
        c->movdqa(dst0, src0);
        break;
      case Operation::CompositeXor:
        doExtractAlpha(a0, src0, 0, 3, true);
        doExtractAlpha(t0, dst0, 0, 3, true);
        doPackedMultiplyAdd2(src0, t0, dst0, a0, t0);
        c->movdqa(dst0, src0);
        break;
      case Operation::CompositeAdd:
        c->paddusb(dst0, src0);
        break;
      case Operation::CompositeSubtract:
        c->psubusb(dst0, src0);
        break;
      case Operation::CompositeMultiply:
        doPackedMultiply2(dst0, src0, t0);
        break;
      case Operation::CompositeSaturate:
      {
        Label* L_Skip = c->newLabel();
        Int32Ref td = f->newVariable(VARIABLE_TYPE_INT32, 0);
        Int32Ref ts = f->newVariable(VARIABLE_TYPE_INT32, 0);

        c->movdqa(t0, dst0);
        c->movdqa(a0, src0);
        c->psrlq(t0, 48);
        c->psrlq(a0, 48);
        c->movd(td.r(), t0);
        c->movd(ts.r(), a0);
        c->neg(td.r());
        c->cmp(ts.r16(), td.r16());
        c->jle(L_Skip);

        c->and_(ts.r(), 0xFF); 
        c->shl(ts.r(), 4);
        c->movd(a0, td.r());
        doExtractAlpha(a0, a0, 0, 0, false);
        c->movdqa(t0, ptr(g->rconst.r(), ts.r(), 0, RCONST_DISP(CxDemultiply)));
        doPackedMultiply2(a0, t0, t0);
        doPackedMultiply2(src0, a0, a0);

        c->bind(L_Skip);
        c->paddusb(dst0, src0);
        break;
      }
    }
  }

  virtual void doUnpackedPixel2(
    const XMMRegister& dst0, const XMMRegister& src0)
  {
  }

  virtual void doUnpackedPixel4(
    const XMMRegister& dst0, const XMMRegister& src0, 
    const XMMRegister& dst1, const XMMRegister& src1)
  {
  }

  UInt32 op;
};

void Generator::blitSpan(const PixelFormat& pfDst, const PixelFormat& pfSrc, const Operation& op)
{
  initConstants();

  c->comment("Generator::blitSpan() - %s <- %s : %s", pfDst.name(), pfSrc.name(), op.name());

  f = c->newFunction(CConv, BuildFunction3<void*, void*, SysUInt>());
  f->setNaked(true);
  f->setAllocableEbp(true);

  // If operation is nop, do nothing - this should be better optimized in frontend
  if (op.id() == Operation::CompositeDest) { c->endFunction(); return; }

  // Destination and source
  PtrRef dst = f->argument(0); 
  PtrRef src = f->argument(1);
  SysIntRef cnt = f->argument(2);

  cnt.setPreferredRegister(REG_NCX);
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
      blitSpan_MemCpy4(dst, src, cnt);
    }
  }
  // [Combine...]
  else
  {
    if (1)
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

      Label* L_Align   = c->newLabel();
      Label* L_Aligned = c->newLabel();
      Label* L_Loop    = c->newLabel();
      Label* L_Tail    = c->newLabel();
      Label* L_Exit    = c->newLabel();

      Int32 mainLoopSize = 16;
      Int32 i;

      CompositeOp32_Over c_op(this, c, f, op.id());
      c_op.init();

      // FIXME: Remoeve it, it's only for testing
      c->jmp(L_Tail);

      // ------------------------------------------------------------------------
      // [Align]
      // ------------------------------------------------------------------------

      // For small size, we will use different smaller loop
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
      c_op.doRawPixel1(dstpix0, srcpix0);
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

      // ------------------------------------------------------------------------
      // [Loop]
      // ------------------------------------------------------------------------

      c->sub(cnt.r(), mainLoopSize / 4);
      c->jc(L_Tail);

      // Loop - Src is Unaligned
      c->align(4);
      c->bind(L_Loop);

      for (i = 0; i < mainLoopSize; i += 16)
      {
        c->movq(srcpix0, ptr(src.r(), i + 0));
        c->movq(srcpix1, ptr(src.r(), i + 8));
        c->movdqa(dstpix0, ptr(dst.r(), i + 0));

        c_op.doRawPixel4(dstpix0, srcpix0, dstpix1, srcpix1,
          CompositeOp32::Raw4UnpackFromDst0 |
          CompositeOp32::Raw4PackToDst0);

        c->movdqa(ptr(dst.r(), i + 0), dstpix0);
      }

      c->add(src.r(), mainLoopSize);
      c->add(dst.r(), mainLoopSize);

      c->sub(cnt.r(), mainLoopSize / 4);
      c->jnc(L_Loop);

      c->add(cnt.r(), mainLoopSize / 4);
      c->jz(L_Exit);

      // ------------------------------------------------------------------------
      // [Tail]
      // ------------------------------------------------------------------------

      c->align(4);
      c->bind(L_Tail);

      c->movd(srcpix0, ptr(src.r()));
      c->movd(dstpix0, ptr(dst.r()));
      c_op.doRawPixel1(dstpix0, srcpix0);
      c->movd(ptr(dst.r()), dstpix0);

      c->add(src.r(), 4);
      c->add(dst.r(), 4);
      c->dec(cnt.r());
      c->jnz(L_Tail);

      c->bind(L_Exit);

      c_op.free();
    }
  }

  c->endFunction();
}

void Generator::blitSpan_MemCpy4(PtrRef& dst, PtrRef& src, SysIntRef& cnt)
{
  // --------------------------------------------------------------------------
  // [MemCopy4 - X86]
  // --------------------------------------------------------------------------
  if (optimize == OptimizeX86)
  {
    Int32Ref pix = f->newVariable(VARIABLE_TYPE_INT32, 0);
    pix.alloc();

    Label* L_Loop = c->newLabel();
    c->bind(L_Loop);

    c->mov(pix.r(), dword_ptr(src.r()));
    c->mov(dword_ptr(dst.r()), pix.r());

    c->add(src.r(), 4);
    c->add(dst.r(), 4);

    c->dec(cnt.r());
    c->jnz(L_Loop);
  }

  // --------------------------------------------------------------------------
  // [MemCopy4 - MMX]
  // --------------------------------------------------------------------------
  else if (optimize == OptimizeMMX)
  {
    SysIntRef t = f->newVariable(VARIABLE_TYPE_SYSINT, 0);

    MMRef _v0 = f->newVariable(VARIABLE_TYPE_MM, 0);
    MMRef _v1 = f->newVariable(VARIABLE_TYPE_MM, 0);

    MMRegister v0 = _v0.r();
    MMRegister v1 = _v1.r();

    Label* L_Aligned = c->newLabel();
    Label* L_Loop    = c->newLabel();
    Label* L_Tail    = c->newLabel();
    Label* L_Exit    = c->newLabel();

    Int32 mainLoopSize = 64;
    Int32 i;

    // ------------------------------------------------------------------------
    // [Align]
    // ------------------------------------------------------------------------

    c->mov(t.r(), dst.r());
    c->and_(t.r(), 7);
    c->jz(L_Aligned);

    c->dec(cnt.r());

    c->movd(v0, ptr(src.r()));
    c->movd(ptr(dst.r()), v0);

    c->add(src.r(), 4);
    c->add(dst.r(), 4);

    c->bind(L_Aligned);

    // ------------------------------------------------------------------------
    // [Loop]
    // ------------------------------------------------------------------------

    c->sub(cnt.r(), mainLoopSize / 4);
    c->jc(L_Tail);

    c->align(4);
    c->bind(L_Loop);

    //c->prefetch(ptr(src.r(), mainLoopSize), PREFETCH_T0);
    //c->prefetch(ptr(dst.r(), mainLoopSize), PREFETCH_NTA);

    for (i = 0; i < mainLoopSize; i += 16)
    {
      c->movq(v0, ptr(src.r(), i + 0));
      c->movq(v1, ptr(src.r(), i + 8));
      c->movq(ptr(dst.r(), i + 0), v0);
      c->movq(ptr(dst.r(), i + 8), v1);
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

    c->align(4);
    c->bind(L_Tail);

    c->movd(v0, ptr(src.r()));
    c->movd(ptr(dst.r()), v0);

    c->add(src.r(), 4);
    c->add(dst.r(), 4);
    c->dec(cnt.r());
    c->jnz(L_Tail);

    c->bind(L_Exit);
  }

  // --------------------------------------------------------------------------
  // [MemCopy4 - SSE2]
  // --------------------------------------------------------------------------
  else if (optimize == OptimizeSSE2)
  {
    SysIntRef t = f->newVariable(VARIABLE_TYPE_SYSINT, 0);

    XMMRef _v0 = f->newVariable(VARIABLE_TYPE_XMM, 0);
    XMMRef _v1 = f->newVariable(VARIABLE_TYPE_XMM, 0);

    XMMRegister v0 = _v0.r();
    XMMRegister v1 = _v1.r();

    Label* L_Align   = c->newLabel();
    Label* L_Aligned = c->newLabel();
    Label* L_Loop    = c->newLabel();
    Label* L_Tail    = c->newLabel();
    Label* L_Exit    = c->newLabel();
    Label* L_Broken  = c->newLabel();
    Label* L_Safe    = c->newLabel();

    Int32 mainLoopSize = 64;
    Int32 i;

    // ------------------------------------------------------------------------
    // [Align]
    // ------------------------------------------------------------------------

    // For small size, we will use different smaller loop
    c->cmp(cnt.r(), 8);
    c->jl(L_Safe);

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
    c->jnz(L_Align);

    // This shouldn't happen, but we must be sure
    c->mov(t.r(), dst.r());
    c->and_(t.r(), 3);
    c->jnz(L_Broken);

    c->bind(L_Aligned);

    // ------------------------------------------------------------------------
    // [Loop]
    // ------------------------------------------------------------------------

    c->sub(cnt.r(), mainLoopSize / 4);
    c->jc(L_Tail);

    c->align(4);
    c->bind(L_Loop);

    //c->prefetch(ptr(src.r(), mainLoopSize), PREFETCH_T0);
    //c->prefetch(ptr(dst.r(), mainLoopSize), PREFETCH_NTA);

    for (i = 0; i < mainLoopSize; i += 32)
    {
      c->movdqu(v0, ptr(src.r(), i +  0));
      c->movdqu(v1, ptr(src.r(), i + 16));
      c->movdqa(ptr(dst.r(), i +  0), v0);
      c->movdqa(ptr(dst.r(), i + 16), v1);
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

    c->align(4);
    c->bind(L_Tail);

    c->movd(v0, ptr(src.r()));
    c->movd(ptr(dst.r()), v0);

    c->add(src.r(), 4);
    c->add(dst.r(), 4);
    c->dec(cnt.r());
    c->jnz(L_Tail);
    c->jmp(L_Exit);

    // ------------------------------------------------------------------------
    // [Safe]
    // ------------------------------------------------------------------------

    c->align(4);
    c->bind(L_Safe);

    c->movd(v0, ptr(src.r()));
    c->movd(ptr(dst.r()), v0);

    c->add(src.r(), 4);
    c->add(dst.r(), 4);
    c->dec(cnt.r());
    c->jnz(L_Safe);

    c->bind(L_Exit);
  }
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

    c->Cx00000000000001000000000000000100.set_uw(0x0000, 0x0000, 0x0000, 0x0100, 0x0000, 0x0000, 0x0000, 0x0100);
    c->Cx00800080008000800080008000800080.set_uw(0x0080, 0x0080, 0x0080, 0x0080, 0x0080, 0x0080, 0x0080, 0x0080);
    c->Cx00FF00FF00FF00FF00FF00FF00FF00FF.set_uw(0x00FF, 0x00FF, 0x00FF, 0x00FF, 0x00FF, 0x00FF, 0x00FF, 0x00FF);
    c->Cx000000000000FFFF000000000000FFFF.set_uw(0x0000, 0x0000, 0x0000, 0xFFFF, 0x0000, 0x0000, 0x0000, 0xFFFF);
    c->CxFFFFFFFFFFFF0100FFFFFFFFFFFF0100.set_uw(0xFFFF, 0xFFFF, 0xFFFF, 0x0100, 0xFFFF, 0xFFFF, 0xFFFF, 0x0100);

    for (i = 0; i < 256; i++)
    {
      UInt16 x = i > 0 ? (int)((256.0 * 255.0) / (float)i + 0.5) : 0;
      c->CxDemultiply[i].set_uw(i, i, i, i, i, i, i, i);
    }

    constants = c;
  }
}

} // BlitJit namespace
