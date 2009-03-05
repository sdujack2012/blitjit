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

#include "BlitJitApi.h"
#include "BlitJitGenerator.h"

#include <AsmJit/AsmJitAssembler.h>
#include <AsmJit/AsmJitCompiler.h>
#include <AsmJit/AsmJitCpuInfo.h>

using namespace AsmJit;

namespace BlitJit {

// ============================================================================
// [BlitJit::Macros]
// ============================================================================

#define RCONST_DISP(__cname__) \
  (Int32)( (UInt8 *)&Generator::constants->__cname__ - (UInt8 *)Generator::constants )

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
// [BlitJit::GeneratorOpCompositeOp32]
// ============================================================================

//! @brief Implementation of compositing operations for SSE2 instruction set.
struct GeneratorOp_Composite32_SSE2 : public GeneratorOp
{
  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  GeneratorOp_Composite32_SSE2(Generator* g, Compiler* c, Function* f, UInt32 op) : GeneratorOp(g, c, f), op(op), maxPixelsInLoop(0)
  {
    maxPixelsInLoop = 4;

    if (op == Operation::CompositeSubtract  ) maxPixelsInLoop = 2;
    if (op == Operation::CompositeMultiply  ) maxPixelsInLoop = 2;
    if (op == Operation::CompositeScreen    ) maxPixelsInLoop = 2;
    if (op == Operation::CompositeDarken    ) maxPixelsInLoop = 2;
    if (op == Operation::CompositeLighten   ) maxPixelsInLoop = 2;
    if (op == Operation::CompositeDifference) maxPixelsInLoop = 2;
    if (op == Operation::CompositeExclusion ) maxPixelsInLoop = 2;
    if (op == Operation::CompositeInvert    ) maxPixelsInLoop = 2;
    if (op == Operation::CompositeInvertRgb ) maxPixelsInLoop = 2;
    if (op == Operation::CompositeSaturate  ) maxPixelsInLoop = 1;
  }

  virtual ~GeneratorOp_Composite32_SSE2() {}

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
    c0x0080.use(f->newVariable(VARIABLE_TYPE_XMM, 0));

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

    c->punpcklbw(src0, z.r());
    c->punpcklbw(dst0, z.r());

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

  // --------------------------------------------------------------------------
  // [doPixelUnpacked]
  // --------------------------------------------------------------------------

  void doPixelUnpacked(
    const XMMRegister& dst0, const XMMRegister& src0, bool two)
  {
    XMMRef _t0(f->newVariable(VARIABLE_TYPE_XMM, 0));
    XMMRef _t1(f->newVariable(VARIABLE_TYPE_XMM, 0));

    XMMRegister t0 = _t0.r();
    XMMRegister t1 = _t1.r();

    switch (op)
    {
      // Dca' = Sca
      // Da'  = Sa
      case Operation::CompositeSrc:
      {
        // copy operation (optimized in frontends and also by Generator itself)
        c->movdqa(dst0, src0);
        break;
      }

      // Dca' = Dca
      // Da'  = Da
      case Operation::CompositeDest:
      {
        // no operation (optimized in frontends and also by Generator itself)
        break;
      }

      // Dca' = Sca + Dca.(1 - Sa)
      // Da'  = Sa + Da.(1 - Sa)
      //      = Sa + Da - Sa.Da
      case Operation::CompositeOver:
      {
        doExtractAlpha(t0, src0, 3, true, two);
        doPackedMultiply(dst0, t0, t0);
        c->paddusb(dst0, src0);
        break;
      }

      // Dca' = Dca + Sca.(1 - Da)
      // Da'  = Da + Sa.(1 - Da)
      //      = Da + Sa - Da.Sa
      case Operation::CompositeOverReverse:
      {
        doExtractAlpha(t0, dst0, 3, true, two);
        doPackedMultiply(src0, t0, t0);
        c->paddusb(dst0, src0);
        break;
      }

      // Dca' = Sca.Da
      // Da'  = Sa.Da 
      case Operation::CompositeIn:
      {
        doExtractAlpha(t0, dst0, 3, false, two);
        doPackedMultiply(src0, t0, dst0, true);
        break;
      }

      // Dca' = Dca.Sa
      // Da'  = Da.Sa
      case Operation::CompositeInReverse:
      {
        doExtractAlpha(t0, src0, 3, false, two);
        doPackedMultiply(dst0, t0, t0);
        break;
      }

      // Dca' = Sca.(1 - Da)
      // Da'  = Sa.(1 - Da) 
      case Operation::CompositeOut:
      {
        doExtractAlpha(t0, dst0, 3, true, two);
        doPackedMultiply(src0, t0, dst0, true);
        break;
      }

      // Dca' = Dca.(1 - Sa) 
      // Da'  = Da.(1 - Sa) 
      case Operation::CompositeOutReverse:
      {
        doExtractAlpha(t0, src0, 3, true, two);
        doPackedMultiply(dst0, t0, t0);
        break;
      }

      // Dca' = Sca.Da + Dca.(1 - Sa)
      // Da'  = Sa.Da + Da.(1 - Sa)
      //      = Da.(Sa + 1 - Sa)
      //      = Da
      case Operation::CompositeAtop:
      {
        doExtractAlpha(t0, src0, 3, true, two);
        doExtractAlpha(t1, dst0, 3, false, two);
        doPackedMultiplyAdd(src0, t1, dst0, t0, dst0, true);
        break;
      }

      // Dca' = Dca.Sa + Sca.(1 - Da)
      // Da'  = Da.Sa + Sa.(1 - Da)
      //      = Sa.(Da + 1 - Da)
      //      = Sa 
      case Operation::CompositeAtopReverse:
      {
        doExtractAlpha(t0, src0, 3, false, two);
        doExtractAlpha(t1, dst0, 3, true, two);
        doPackedMultiplyAdd(src0, t1, dst0, t0, dst0, true);
        break;
      }

      // Dca' = Sca.(1 - Da) + Dca.(1 - Sa)
      // Da'  = Sa.(1 - Da) + Da.(1 - Sa)
      //      = Sa + Da - 2.Sa.Da 
      case Operation::CompositeXor:
      {
        doExtractAlpha(t0, src0, 3, true, two);
        doExtractAlpha(t1, dst0, 3, true, two);
        doPackedMultiplyAdd(
          src0, t1, 
          dst0, t0, 
          dst0, true);
        break;
      }

      // Dca' = 0
      // Da'  = 0
      case Operation::CompositeClear:
      {
        // clear operation (optimized in frontends and also by Generator itself)
        c->pxor(dst0, dst0);
        break;
      }

      // Dca' = Sca + Dca
      // Da'  = Sa + Da 
      case Operation::CompositeAdd:
      {
        c->paddusb(dst0, src0);
        break;
      }

      // Dca' = Dca - Sca
      // Da'  = 1 - (1 - Sa).(1 - Da)
      case Operation::CompositeSubtract:
      {
        doExtractAlpha(t0, src0, 3, true, two);
        doExtractAlpha(t1, dst0, 3, true, two);
        doPackedMultiply(t0, t1, t1);
        c->psubusb(dst0, src0);
        c->por(dst0, ptr(g->rconst.r(), RCONST_DISP(Cx00FF00000000000000FF000000000000)));
        c->pand(t0, ptr(g->rconst.r(), RCONST_DISP(Cx00FF00000000000000FF000000000000)));
        c->psubusb(dst0, t0);
        break;
      }

      // Dca' = Sca.Dca + Sca.(1 - Da) + Dca.(1 - Sa)
      // Da'  = Sa.Da + Sa.(1 - Da) + Da.(1 - Sa)
      //      = Sa + Da - Sa.Da 
      case Operation::CompositeMultiply:
      {
        XMMRef _t2(f->newVariable(VARIABLE_TYPE_XMM, 0));
        XMMRegister t2 = _t2.r();

        c->movdqa(t2, dst0);
        doPackedMultiply(t2, dst0, t0);

        doExtractAlpha(t0, src0, 3, true, two); // t0 == 1-alpha from src0
        doExtractAlpha(t1, dst0, 3, true, two); // t1 == 1-alpha from dst0

        doPackedMultiplyAdd(dst0, t0, src0, t1, t0);
        c->paddusw(dst0, t2);
        break;
      }

      // Dca' = (Sca.Da + Dca.Sa - Sca.Dca) + Sca.(1 - Da) + Dca.(1 - Sa)
      //      = Sca + Dca - Sca.Dca
      // Da'  = Sa + Da - Sa.Da 
      case Operation::CompositeScreen:
      {
        c->movdqa(t0, dst0);
        c->paddusw(dst0, src0);
        doPackedMultiply(t0, src0, t1);
        c->psubusw(dst0, t0);
        break;
      }

      // Dca' = min(Sca.Da, Dca.Sa) + Sca.(1 - Da) + Dca.(1 - Sa)
      // Da'  = min(Sa.Da, Da.Sa) + Sa.(1 - Da) + Da.(1 - Sa)
      //      = Sa.Da + Sa - Sa.Da + Da - Sa.Da
      //      = Sa + Da - Sa.Da 
      case Operation::CompositeDarken:

      // Dca' = max(Sca.Da, Dca.Sa) + Sca.(1 - Da) + Dca.(1 - Sa)
      // Da'  = max(Sa.Da, Da.Sa) + Sa.(1 - Da) + Da.(1 - Sa)
      //      = Sa.Da + Sa - Sa.Da + Da - Sa.Da
      //      = Sa + Da - Sa.Da 
      case Operation::CompositeLighten:
      {
        XMMRef _t2(f->newVariable(VARIABLE_TYPE_XMM, 0));
        XMMRef _t3(f->newVariable(VARIABLE_TYPE_XMM, 0));

        XMMRegister t2 = _t2.r();
        XMMRegister t3 = _t3.r();

        doExtractAlpha_4(
          t0, src0, 3, false,
          t1, dst0, 3, false);
        doPackedMultiply_4(
          t0, dst0, t2,
          t1, src0, t3, true);

        if (op == Operation::CompositeDarken)
          c->pminsw(t3, t2);
        else
          c->pmaxsw(t3, t2);

        doExtractAlpha_4(
          t0, src0, 3, true,
          t1, dst0, 3, true);
        doPackedMultiplyAdd(
          dst0, t0,
          src0, t1, t2);
        c->paddusw(dst0, t3);
        break;
      }

      // Dca' = Sca + Dca - 2.min(Sca.Da, Dca.Sa)
      // Da'  = Sa + Da - min(Sa.Da, Da.Sa)
      //      = Sa + Da - Sa.Da
      case Operation::CompositeDifference:
      {
        XMMRef _t2(f->newVariable(VARIABLE_TYPE_XMM, 0));
        XMMRef _t3(f->newVariable(VARIABLE_TYPE_XMM, 0));

        XMMRegister t2 = _t2.r();
        XMMRegister t3 = _t3.r();

        doExtractAlpha_4(
          t0, src0, 3, false,
          t1, dst0, 3, false);
        doPackedMultiply_4(
          t0, dst0, t2,
          t1, src0, t3);
        c->pminsw(t0, t1);
        c->paddusw(dst0, src0);
        c->psubusw(dst0, t0);
        c->pand(t0, ptr(g->rconst.r(), RCONST_DISP(Cx000000FF00FF00FF000000FF00FF00FF)));
        c->psubusw(dst0, t0);
        break;
      }

      // Dca' = (Sca.Da + Dca.Sa - 2.Sca.Dca) + Sca.(1 - Da) + Dca.(1 - Sa)
      // Dca' = Sca + Dca - 2.Sca.Dca
      //      
      // Da'  = (Sa.Da + Da.Sa - 2.Sa.Da) + Sa.(1 - Da) + Da.(1 - Sa)
      //      = Sa - Sa.Da + Da - Da.Sa = Sa + Da - 2.Sa.Da
      // Substitute 2.Sa.Da with Sa.Da
      //
      // Da'  = Sa + Da - Sa.Da 
      case Operation::CompositeExclusion:
      {
        c->movdqa(t0, src0);
        doPackedMultiply(t0, dst0, t1);
        c->paddusw(dst0, src0);
        c->psubusw(dst0, t0);
        c->pand(t0, ptr(g->rconst.r(), RCONST_DISP(Cx000000FF00FF00FF000000FF00FF00FF)));
        c->psubusw(dst0, t0);
        break;
      }

      // Dca' = (Da - Dca) * Sa + Dca.(1 - Sa)
      // Da'  = Sa + (Da - Da) * Sa + Da - Sa.Da
      //      = Sa + Da - Sa.Da
      case Operation::CompositeInvert:
      {
        XMMRef _t2(f->newVariable(VARIABLE_TYPE_XMM, 0));
        XMMRef _t3(f->newVariable(VARIABLE_TYPE_XMM, 0));

        XMMRegister t2 = _t2.r();
        XMMRegister t3 = _t3.r();

        doExtractAlpha_4(
          t0, src0, 3, false,  // Sa
          t2, dst0, 3, false); // Da

        c->movdqa(t1, t0);
        c->psubusb(t2, dst0);
        c->pxor(t1, ptr(g->rconst.r(), RCONST_DISP(Cx00FF00FF00FF00FF00FF00FF00FF00FF)));

        // t1 = 1 - Sa
        // t2 = Da - Dca

        doPackedMultiply_4(
          t2, t0, t3,     // t2   = Sa.(Da - Dca)
          dst0, t1, t1);  // dst0 = Dca.(1 - Sa)

        c->pand(t0, ptr(g->rconst.r(), RCONST_DISP(Cx00FF00000000000000FF000000000000)));
        c->paddusb(dst0, t2);
        c->paddusb(dst0, t0);
        break;
      }

      // Dca' = (Da - Dca) * Sca + Dca.(1 - Sa)
      // Da'  = Sa + (Da - Da) * Sa + Da - Da.Sa
      //      = Sa + Da - Da.Sa
      case Operation::CompositeInvertRgb:
      {
        XMMRef _t2(f->newVariable(VARIABLE_TYPE_XMM, 0));
        XMMRef _t3(f->newVariable(VARIABLE_TYPE_XMM, 0));

        XMMRegister t2 = _t2.r();
        XMMRegister t3 = _t3.r();

        doExtractAlpha_4(
          t0, src0, 3, false,  // Sa
          t2, dst0, 3, false); // Da

        c->movdqa(t1, t0);
        c->psubw(t2, dst0);
        c->pxor(t1, ptr(g->rconst.r(), RCONST_DISP(Cx00FF00FF00FF00FF00FF00FF00FF00FF)));

        // t1 = 1 - Sa
        // t2 = Da - Dca

        doPackedMultiply_4(
          t2, src0, t3,   // t2   = Sca.(Da - Dca)
          dst0, t1, t1);  // dst0 = Dca.(1 - Sa)

        c->pand(t0, ptr(g->rconst.r(), RCONST_DISP(Cx00FF00000000000000FF000000000000)));
        c->paddusb(dst0, t2);
        c->paddusb(dst0, t0);
        break;
      }

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
        c->xor_(td.r(), 0xFF);
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
      case Operation::CompositeMultiply:
      case Operation::CompositeScreen:
      case Operation::CompositeDarken:
      case Operation::CompositeLighten:
      case Operation::CompositeDifference:
      case Operation::CompositeExclusion:
      case Operation::CompositeInvert:
      case Operation::CompositeInvertRgb:
      case Operation::CompositeSaturate:
      {
        BLITJIT_ASSERT(0);
        break;
      }
    }
  }

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

  // moveToT0 false:
  //   a0 = (a0 * b0) / 255, t0 is destroyed.
  // moveToT0 true:
  //   t0 = (a0 * b0) / 255, a0 is destroyed.
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

  void doPackedMultiplyWithAddition(
    const XMMRegister& a0, const XMMRegister& b0, const XMMRegister& t0)
  {
    XMMRegister r = c0x0080.r();

    c->movdqa(t0, a0);
    c->pmullw(a0, b0);          // a0 *= b0
    c->paddusw(a0, r);          // a0 += 80
    c->paddusw(b0, t0);
    c->movdqa(t0, a0);          // t0  = a0
    c->psrlw(a0, 8);            // a0 /= 256
    c->paddusw(a0, t0);         // a0 += t0
    c->psrlw(a0, 8);            // a0 /= 256
    c->paddusw(a0, b0);
  }

  // moveToT0T1 false: 
  //   a0 = (a0 * b0) / 255, t0 is destroyed.
  //   a1 = (a1 * b1) / 255, t1 is destroyed.
  // moveToT0T1 true: 
  //   t0 = (a0 * b0) / 255, a0 is destroyed.
  //   t1 = (a1 * b1) / 255, a1 is destroyed.
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

      c->paddusw(a0, r);          // a0 += 80
      c->paddusw(a1, r);          // a1 += 80

      c->movdqa(t0, a0);          // t0  = a0
      c->psrlw(a0, 8);            // a0 /= 256
      c->movdqa(t1, a1);          // t1  = a1
      c->psrlw(a1, 8);            // a1 /= 256

      if (moveToT0T1)
      {
        c->paddusw(t0, a0);       // t0 += a0
        c->psrlw(t0, 8);          // t0 /= 256

        c->paddusw(t1, a1);       // t1 += a1
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

  // moveToT0 false:
  //   a0 = (a0 * b0 + c0 * d0) / 255, c0 and t0 are destroyed
  // moveToT0 true:
  //   t0 = (a0 * b0 + c0 * d0) / 255, a0 and c0 are destroyed
  void doPackedMultiplyAdd(
    const XMMRegister& a0, const XMMRegister& b0,
    const XMMRegister& c0, const XMMRegister& d0,
    const XMMRegister& t0, bool moveToT0 = false)
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
  UInt32 maxPixelsInLoop;

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
  prefetch(1),
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

void Generator::genFillSpan(const PixelFormat& pfDst, const PixelFormat& pfSrc, const Operation& op)
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
  dst.setPriority(0);
  src.setPriority(0);

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
    // Simple fill should be maxPixelsInLooped loop for 32 or more bytes at a time.
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

void Generator::genFillRect(const PixelFormat& pfDst, const PixelFormat& pfSrc, const Operation& op)
{
}

void Generator::genBlitSpan(const PixelFormat& pfDst, const PixelFormat& pfSrc, const Operation& op)
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
    if (optimize == OptimizeX86)
    {
      // TODO
    }
    else if (optimize == OptimizeMMX)
    {
      // TODO
    }
    else if (optimize == OptimizeSSE2)
    {
      GeneratorOp_Composite32_SSE2 c_op(this, c, f, op.id());
      c_op.init();
      _Composite32_SSE2(dst, src, cnt, c_op);
      c_op.free();
    }
  }

  c->endFunction();
}

void Generator::genBlitRect(const PixelFormat& pfDst, const PixelFormat& pfSrc, const Operation& op)
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
      GeneratorOp_Composite32_SSE2 c_op(this, c, f, op.id());
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

// ============================================================================
// [BlitJit::Generator - MemSet32]
// ============================================================================

void Generator::_MemSet32(PtrRef& dst, Int32Ref& src, SysIntRef& cnt)
{
  StateRef state(f->saveState());

  SysIntRef tmp(f->newVariable(VARIABLE_TYPE_SYSINT, 0));
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

// ============================================================================
// [BlitJit::Generator - MemCpy32]
// ============================================================================

void Generator::_MemCpy32(PtrRef& dst, PtrRef& src, SysIntRef& cnt)
{
  StateRef state(f->saveState());

  // --------------------------------------------------------------------------
  // [MemCopy32 - X86]
  // --------------------------------------------------------------------------
  if (optimize == OptimizeX86)
  {
    Int32Ref tmp(f->newVariable(VARIABLE_TYPE_INT32, 0));

    Label* L_Loop = c->newLabel();
    c->bind(L_Loop);

    c->mov(tmp.r(), dword_ptr(src.r()));
    c->add(src.r(), 4);
    stream_mov(dword_ptr(dst.r()), tmp.r());
    c->add(dst.r(), 4);

    c->dec(cnt.r());
    c->jnz(L_Loop);
  }

  // --------------------------------------------------------------------------
  // [MemCopy32 - MMX]
  // --------------------------------------------------------------------------
  else if (optimize == OptimizeMMX)
  {
    SysIntRef t(f->newVariable(VARIABLE_TYPE_SYSINT, 0));

    MMRef _v0(f->newVariable(VARIABLE_TYPE_MM, 0));
    MMRef _v1(f->newVariable(VARIABLE_TYPE_MM, 0));
    MMRef _v2(f->newVariable(VARIABLE_TYPE_MM, 0));
    MMRef _v3(f->newVariable(VARIABLE_TYPE_MM, 0));

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
    stream_mov(ptr(dst.r()), t.r32());
    c->add(dst.r(), 4);

    c->bind(L_Aligned);

    // ------------------------------------------------------------------------
    // [Loop]
    // ------------------------------------------------------------------------

    c->sub(cnt.r(), mainLoopSize / 4);
    c->jc(L_Tail, HINT_NOT_TAKEN);

    c->align(16);
    c->bind(L_Loop);

    if (prefetch) 
    {
      c->prefetch(ptr(src.r(), mainLoopSize), PREFETCH_T0);
    }

    for (i = 0; i < mainLoopSize; i += 32)
    {
      c->movq(v0, ptr(src.r(), i +  0));
      c->movq(v1, ptr(src.r(), i +  8));
      c->movq(v2, ptr(src.r(), i + 16));
      c->movq(v3, ptr(src.r(), i + 24));
      stream_movq(ptr(dst.r(), i +  0), v0);
      stream_movq(ptr(dst.r(), i +  8), v1);
      stream_movq(ptr(dst.r(), i + 16), v2);
      stream_movq(ptr(dst.r(), i + 24), v3);
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
    c->sub(cnt.r(), 2);
    c->jc(L_TailEnd);

    c->bind(L_TailLoop);

    c->movq(v0, ptr(src.r()));
    c->add(src.r(), 8);
    stream_movq(ptr(dst.r()), v0);
    c->add(dst.r(), 8);

    c->sub(cnt.r(), 2);
    c->jnc(L_TailLoop);

    c->bind(L_TailEnd);

    c->add(cnt.r(), 2);
    c->jz(L_Exit);

    c->mov(t.r32(), dword_ptr(src.r()));
    c->add(src.r(), 4);
    stream_mov(dword_ptr(dst.r()), t.r32());
    c->add(dst.r(), 4);

    c->bind(L_Exit);
  }

  // --------------------------------------------------------------------------
  // [MemCopy32 - SSE2]
  // --------------------------------------------------------------------------
  else if (optimize == OptimizeSSE2)
  {
    SysIntRef t(f->newVariable(VARIABLE_TYPE_SYSINT, 0));

    XMMRef _v0(f->newVariable(VARIABLE_TYPE_XMM, 0));
    XMMRef _v1(f->newVariable(VARIABLE_TYPE_XMM, 0));
    XMMRef _v2(f->newVariable(VARIABLE_TYPE_XMM, 0));
    XMMRef _v3(f->newVariable(VARIABLE_TYPE_XMM, 0));

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
    Label* L_Loop_A  = c->newLabel();
    Label* L_Exit    = c->newLabel();

    Int32 mainLoopSize = 64;
    Int32 i;

    // ------------------------------------------------------------------------
    // [Align]
    // ------------------------------------------------------------------------

    // For smaller size, we will use tail loop
    c->cmp(cnt.r(), 16);
    c->jl(L_Tail, HINT_NOT_TAKEN);

    // This shouldn't happen, but we must be sure
    c->mov(t.r(), 3);
    c->and_(t.r(), dst.r());
    c->jnz(L_Tail, HINT_NOT_TAKEN);

    // Align 1
    c->mov(t.r(), 15);
    c->and_(t.r(), dst.r());
    c->jz(L_Aligned);

    c->mov(t.r32(), dword_ptr(src.r()));
    c->add(src.r(), 4);
    stream_mov(dword_ptr(dst.r()), t.r32());
    c->add(dst.r(), 4);
    c->dec(cnt.r());

    // Align 2
    c->mov(t.r(), 15);
    c->and_(t.r(), dst.r());
    c->jz(L_Aligned);

    c->mov(t.r32(), dword_ptr(src.r()));
    c->add(src.r(), 4);
    stream_mov(dword_ptr(dst.r()), t.r32());
    c->add(dst.r(), 4);
    c->dec(cnt.r());

    // Align 3
    c->mov(t.r(), 15);
    c->and_(t.r(), dst.r());
    c->jz(L_Aligned);

    c->mov(t.r32(), dword_ptr(src.r()));
    c->add(src.r(), 4);
    stream_mov(dword_ptr(dst.r()), t.r32());
    c->add(dst.r(), 4);
    c->dec(cnt.r());

    c->bind(L_Aligned);

    c->sub(cnt.r(), mainLoopSize / 4);
    c->jc(L_Tail, HINT_NOT_TAKEN);

    c->mov(t.r(), 15);
    c->and_(t.r(), src.r());
    c->jz(L_Loop_A);

    // ------------------------------------------------------------------------
    // [Loop]
    // ------------------------------------------------------------------------

    c->align(16);
    c->bind(L_Loop);

    if (prefetch) 
    {
      c->prefetch(ptr(src.r(), mainLoopSize), PREFETCH_T0);
    }

    for (i = 0; i < mainLoopSize; i += 64)
    {
      c->movdqu(v0, ptr(src.r(), i +  0));
      c->movdqu(v1, ptr(src.r(), i + 16));
      c->movdqu(v2, ptr(src.r(), i + 32));
      c->movdqu(v3, ptr(src.r(), i + 48));

      stream_movdq(ptr(dst.r(), i +  0), v0);
      stream_movdq(ptr(dst.r(), i + 16), v1);
      stream_movdq(ptr(dst.r(), i + 32), v2);
      stream_movdq(ptr(dst.r(), i + 48), v3);
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
    c->sub(cnt.r(), 2);
    c->jc(L_TailEnd);

    c->bind(L_TailLoop);
#if defined(ASMJIT_X86)
    c->mov(t.r32(), dword_ptr(src.r()));
    stream_mov(dword_ptr(dst.r()), t.r32());
    c->mov(t.r32(), dword_ptr(src.r(), 4));
    c->add(src.r(), 8);
    stream_mov(dword_ptr(dst.r(), 4), t.r32());
    c->add(dst.r(), 8);
#else
    c->mov(t.r64(), qword_ptr(src.r()));
    c->add(src.r(), 8);
    stream_mov(qword_ptr(dst.r()), t.r64());
    c->add(dst.r(), 8);
#endif
    c->sub(cnt.r(), 2);
    c->jnc(L_TailLoop);

    c->bind(L_TailEnd);
    c->add(cnt.r(), 2);
    c->jz(L_Exit);

    c->mov(t.r32(), dword_ptr(src.r()));
    c->add(src.r(), 4);
    stream_mov(dword_ptr(dst.r()), t.r32());
    c->add(dst.r(), 4);
    c->jmp(L_Exit);

    // Aligned Loop (Fastest possible)
    c->align(16);
    c->bind(L_Loop_A);
    if (prefetch) 
    {
      c->prefetch(ptr(src.r(), mainLoopSize), PREFETCH_T0);
    }

    for (i = 0; i < mainLoopSize; i += 64)
    {
      c->movdqa(v0, ptr(src.r(), i +  0));
      c->movdqa(v1, ptr(src.r(), i + 16));
      c->movdqa(v2, ptr(src.r(), i + 32));
      c->movdqa(v3, ptr(src.r(), i + 48));
      stream_movdq(ptr(dst.r(), i +  0), v0);
      stream_movdq(ptr(dst.r(), i + 16), v1);
      stream_movdq(ptr(dst.r(), i + 32), v2);
      stream_movdq(ptr(dst.r(), i + 48), v3);
    }

    c->add(src.r(), mainLoopSize);
    c->add(dst.r(), mainLoopSize);

    c->sub(cnt.r(), mainLoopSize / 4);
    c->jnc(L_Loop_A);

    c->add(cnt.r(), mainLoopSize / 4);
    c->jnz(L_Tail);

    c->bind(L_Exit);
  }
}

// ============================================================================
// [BlitJit::Generator - Composite32]
// ============================================================================

void Generator::_Composite32_SSE2(
  PtrRef& dst, PtrRef& src, SysIntRef& cnt,
  GeneratorOp_Composite32_SSE2& c_op)
{
  StateRef state(f->saveState());

  SysIntRef t(f->newVariable(VARIABLE_TYPE_SYSINT, 0));

  XMMRef _dstpix0(f->newVariable(VARIABLE_TYPE_XMM, 0));
  XMMRef _dstpix1(f->newVariable(VARIABLE_TYPE_XMM, 0));
  XMMRef _srcpix0(f->newVariable(VARIABLE_TYPE_XMM, 0));
  XMMRef _srcpix1(f->newVariable(VARIABLE_TYPE_XMM, 0));

  XMMRegister srcpix0 = _srcpix0.r();
  XMMRegister dstpix0 = _dstpix0.r();

  Label* L_Align   = c->newLabel();
  Label* L_Aligned = c->newLabel();
  Label* L_Loop    = c->newLabel();
  Label* L_Tail    = c->newLabel();
  Label* L_Exit    = c->newLabel();

  Int32 mainLoopSize = 16;

  Int32 i;
  UInt32 maxPixelsInLoop = c_op.maxPixelsInLoop;

  if (maxPixelsInLoop) t.alloc();

  // ------------------------------------------------------------------------
  // [Align]
  // ------------------------------------------------------------------------

  if (maxPixelsInLoop >= 2)
  {
    // For small size, we will use tail loop
    if (maxPixelsInLoop >= 4)
    {
      c->cmp(cnt.r(), 4);
      c->jl(L_Tail);

      c->xor_(t.r(), t.r());
      c->sub(t.r(), dst.r());
      c->and_(t.r(), 15);
      c->jz(L_Aligned);

      c->shr(t.r(), 2);
      c->sub(cnt.r(), t.r());
    }
    else
    {
      c->mov(t.r(), dst.r());
      c->and_(t.r(), 7);
      c->jz(L_Aligned);

      c->dec(t.r());
    }

    c->bind(L_Align);

    c->movd(srcpix0, ptr(src.r()));
    c->movd(dstpix0, ptr(dst.r()));
    c_op.doPixelRaw(dstpix0, srcpix0, false);
    c->movd(ptr(dst.r()), dstpix0);

    c->add(src.r(), 4);
    c->add(dst.r(), 4);

    if (maxPixelsInLoop >= 4)
    {
      c->dec(t.r());
      c->jnz(L_Align);

      // This shouldn't happen, because we are expecting 16 byte alignment in 
      // main loop, but we must be sure!
      c->mov(t.r(), dst.r());
      c->and_(t.r(), 3);
      c->jnz(L_Tail);
    }

    c->bind(L_Aligned);
  }

  // ------------------------------------------------------------------------
  // [Loop]
  // ------------------------------------------------------------------------

  if (maxPixelsInLoop >= 4)
  {
    XMMRegister dstpix1 = _dstpix1.r();
    XMMRegister srcpix1 = _srcpix1.r();

    c->sub(cnt.r(), mainLoopSize / 4);
    c->jc(L_Tail);

    // Loop - Src is Unaligned
    c->align(16);
    c->bind(L_Loop);

    if (prefetch) 
    {
      c->prefetch(ptr(src.r(), mainLoopSize), PREFETCH_T0);
      c->prefetch(ptr(dst.r(), mainLoopSize), PREFETCH_T0);
    }

    for (i = 0; i < mainLoopSize; i += 16)
    {
      Label *L_LocalLoopExit = c->newLabel();

      c->movdqu(srcpix0, ptr(src.r(), i + 0));

      // This can improve speed by skipping pixels of zero or full alpha
      if (c_op.op == Operation::CompositeOver)
      {
        Label *L_1 = c->newLabel();
        SysIntRef k = f->newVariable(VARIABLE_TYPE_SYSINT, 0);

        c->pcmpeqb(dstpix0, dstpix0);
        c->pxor(dstpix1, dstpix1);

        c->pcmpeqb(dstpix0, srcpix0);
        c->pcmpeqb(dstpix1, srcpix0);

        c->pmovmskb(k.r32(), dstpix0);
        c->pmovmskb(t.r32(), dstpix1);

        c->and_(k.r32(), 0x8888);
        c->cmp(t.r32(), 0xFFFF);
        c->jz(L_LocalLoopExit);

        c->cmp(k.r32(), 0x8888);
        c->jnz(L_1);
        c->movdqa(ptr(dst.r(), i + 0), srcpix0);
        c->jmp(L_LocalLoopExit);

        c->bind(L_1);
      }

      c->movdqa(dstpix0, ptr(dst.r(), i + 0));

      // Source and destination is in srcpix0 and dstpix0, also we want to
      // pack destination to dstpix0.
      c_op.doPixelRaw_4(dstpix0, srcpix0, dstpix1, srcpix1,
        GeneratorOp_Composite32_SSE2::Raw4UnpackFromSrc0 |
        GeneratorOp_Composite32_SSE2::Raw4UnpackFromDst0 |
        GeneratorOp_Composite32_SSE2::Raw4PackToDst0);

      c->movdqa(ptr(dst.r(), i + 0), dstpix0);

      c->bind(L_LocalLoopExit);
    }
    c->add(src.r(), mainLoopSize);
    c->add(dst.r(), mainLoopSize);

    c->sub(cnt.r(), mainLoopSize / 4);
    c->jnc(L_Loop, HINT_TAKEN);

    c->add(cnt.r(), mainLoopSize / 4);
    c->jz(L_Exit);
  }

  // ------------------------------------------------------------------------
  // [Tail]
  // ------------------------------------------------------------------------

  if (maxPixelsInLoop >= 2)
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
// [BlitJit::Generator - Streaming]
// ============================================================================

void Generator::stream_mov(const Mem& dst, const Register& src)
{
  if (nonThermalHint && (features & AsmJit::CpuInfo::Feature_SSE2) != 0)
  {
    c->movnti(dst, src);
  }
  else
  {
    c->mov(dst, src);
  }
}

void Generator::stream_movq(const Mem& dst, const MMRegister& src)
{
  if (nonThermalHint && (features & (AsmJit::CpuInfo::Feature_SSE | 
                                     AsmJit::CpuInfo::Feature_3dNow)) != 0)
  {
    c->movntq(dst, src);
  }
  else
  {
    c->movq(dst, src);
  }
}

void Generator::stream_movdq(const Mem& dst, const XMMRegister& src)
{
  if (nonThermalHint)
  {
    c->movntdq(dst, src);
  }
  else
  {
    c->movdqa(dst, src);
  }
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

    for (i = 0; i < 256; i++)
    {
      UInt16 x = i > 0 ? (int)((256.0 * 255.0) / (float)i + 0.5) : 0;
      c->CxDemultiply[i].set_uw(i, i, i, i, i, i, i, i);
    }

    constants = c;
  }
}

} // BlitJit namespace
