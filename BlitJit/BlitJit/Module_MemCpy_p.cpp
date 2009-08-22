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

#include "Generator_p.h"
#include "Module_MemCpy_p.h"

using namespace AsmJit;

namespace BlitJit {

// ============================================================================
// [BlitJit::Module_MemCpy32]
// ============================================================================

Module_MemCpy32::Module_MemCpy32(
  Generator* g,
  const PixelFormat* dstPf,
  const PixelFormat* srcPf,
  const Operator* op) :
  Module_Blit(g, dstPf, srcPf, NULL, op)
{
  _prefetchDst = false;
  _prefetchSrc = true;

  switch (g->optimization())
  {
    case OptimizeX86:
      _maxPixelsPerLoop = 4;
      break;
    case OptimizeMMX:
      _maxPixelsPerLoop = 16;
      break;
    case OptimizeSSE2:
      _maxPixelsPerLoop = 32;
      break;
  }
}

Module_MemCpy32::~Module_MemCpy32()
{
}

void Module_MemCpy32::processPixelsPtr(
  const AsmJit::PtrRef* dst,
  const AsmJit::PtrRef* src,
  const AsmJit::PtrRef* msk,
  SysInt count,
  SysInt offset,
  UInt32 kind,
  UInt32 flags)
{
  StateRef state(c->saveState());

  bool nt = g->nonThermalHint();
  bool srcAligned = (flags & SrcAligned) != 0;
  bool dstAligned = (flags & DstAligned) != 0;

  switch (g->optimization())
  {
    case OptimizeX86:
    {
      SysInt i = count;

      do {
        SysInt dstDisp = dstPf->bytesPerPixel() * offset;
        SysInt srcDisp = srcPf->bytesPerPixel() * offset;

#if defined(ASMJIT_X64)
        if (i >= 2)
        {
          SysIntRef t(c->newVariable(VARIABLE_TYPE_INT64));

          g->loadQ(t, qword_ptr(src->c(), dstDisp));
          g->storeQ(qword_ptr(dst->c(), srcDisp), t, nt);

          offset += 2;
          i -= 2;
        }
        else
        {
#endif // ASMJIT_X64
          SysIntRef t(c->newVariable(VARIABLE_TYPE_INT32));

          g->loadD(t, dword_ptr(src->c(), dstDisp));
          g->storeD(dword_ptr(dst->c(), srcDisp), t, nt);

          offset++;
          i--;
#if defined(ASMJIT_X64)
        }
#endif // ASMJIT_X64
      } while (i > 0);
      break;
    }

    case OptimizeMMX:
    {
      SysInt i = count;

      do {
        SysInt dstDisp = dstPf->bytesPerPixel() * offset;
        SysInt srcDisp = srcPf->bytesPerPixel() * offset;

        if (i >= 16 && c->numFreeMm() >= 8)
        {
          MMRef t0(c->newVariable(VARIABLE_TYPE_MM));
          MMRef t1(c->newVariable(VARIABLE_TYPE_MM));
          MMRef t2(c->newVariable(VARIABLE_TYPE_MM));
          MMRef t3(c->newVariable(VARIABLE_TYPE_MM));
          MMRef t4(c->newVariable(VARIABLE_TYPE_MM));
          MMRef t5(c->newVariable(VARIABLE_TYPE_MM));
          MMRef t6(c->newVariable(VARIABLE_TYPE_MM));
          MMRef t7(c->newVariable(VARIABLE_TYPE_MM));

          g->loadQ(t0, qword_ptr(src->c(), dstDisp     ));
          g->loadQ(t1, qword_ptr(src->c(), dstDisp +  8));
          g->loadQ(t2, qword_ptr(src->c(), dstDisp + 16));
          g->loadQ(t3, qword_ptr(src->c(), dstDisp + 24));
          g->loadQ(t4, qword_ptr(src->c(), dstDisp + 32));
          g->loadQ(t5, qword_ptr(src->c(), dstDisp + 40));
          g->loadQ(t6, qword_ptr(src->c(), dstDisp + 48));
          g->loadQ(t7, qword_ptr(src->c(), dstDisp + 56));

          g->storeQ(qword_ptr(dst->c(), srcDisp     ), t0, nt);
          g->storeQ(qword_ptr(dst->c(), srcDisp +  8), t1, nt);
          g->storeQ(qword_ptr(dst->c(), srcDisp + 16), t2, nt);
          g->storeQ(qword_ptr(dst->c(), srcDisp + 24), t3, nt);
          g->storeQ(qword_ptr(dst->c(), srcDisp + 32), t4, nt);
          g->storeQ(qword_ptr(dst->c(), srcDisp + 40), t5, nt);
          g->storeQ(qword_ptr(dst->c(), srcDisp + 48), t6, nt);
          g->storeQ(qword_ptr(dst->c(), srcDisp + 56), t7, nt);

          offset += 16;
          i -= 16;
        }
        else if (i >= 8 && c->numFreeMm() >= 4)
        {
          MMRef t0(c->newVariable(VARIABLE_TYPE_MM));
          MMRef t1(c->newVariable(VARIABLE_TYPE_MM));
          MMRef t2(c->newVariable(VARIABLE_TYPE_MM));
          MMRef t3(c->newVariable(VARIABLE_TYPE_MM));

          g->loadQ(t0, qword_ptr(src->c(), srcDisp     ));
          g->loadQ(t1, qword_ptr(src->c(), srcDisp +  8));
          g->loadQ(t2, qword_ptr(src->c(), srcDisp + 16));
          g->loadQ(t3, qword_ptr(src->c(), srcDisp + 24));

          g->storeQ(qword_ptr(dst->c(), dstDisp     ), t0, nt);
          g->storeQ(qword_ptr(dst->c(), dstDisp +  8), t1, nt);
          g->storeQ(qword_ptr(dst->c(), dstDisp + 16), t2, nt);
          g->storeQ(qword_ptr(dst->c(), dstDisp + 24), t3, nt);

          offset += 8;
          i -= 8;
        }
        else if (i >= 4 && c->numFreeMm() >= 2)
        {
          MMRef t0(c->newVariable(VARIABLE_TYPE_MM));
          MMRef t1(c->newVariable(VARIABLE_TYPE_MM));

          g->loadQ(t0, qword_ptr(src->c(), srcDisp));
          g->loadQ(t1, qword_ptr(src->c(), srcDisp + 8));

          g->storeQ(qword_ptr(dst->c(), dstDisp), t0, nt);
          g->storeQ(qword_ptr(dst->c(), dstDisp + 8), t1, nt);

          offset += 4;
          i -= 4;
        }
        else if (i >= 2)
        {
          MMRef t(c->newVariable(VARIABLE_TYPE_MM));

          g->loadQ(t, qword_ptr(src->c(), srcDisp));
          g->storeQ(qword_ptr(dst->c(), dstDisp), t, nt);

          offset += 2;
          i -= 2;
        }
        else
        {
          SysIntRef t(c->newVariable(VARIABLE_TYPE_INT32));

          g->loadD(t, dword_ptr(src->c(), srcDisp));
          g->storeD(dword_ptr(dst->c(), dstDisp), t, nt);

          offset++;
          i--;
        }
      } while (i > 0);
      break;
    }

    case OptimizeSSE2:
    {
      SysInt i = count;

      do {
        SysInt dstDisp = dstPf->bytesPerPixel() * offset;
        SysInt srcDisp = srcPf->bytesPerPixel() * offset;

        if (i >= 32 && c->numFreeXmm() >= 8)
        {
          XMMRef t0(c->newVariable(VARIABLE_TYPE_XMM));
          XMMRef t1(c->newVariable(VARIABLE_TYPE_XMM));
          XMMRef t2(c->newVariable(VARIABLE_TYPE_XMM));
          XMMRef t3(c->newVariable(VARIABLE_TYPE_XMM));
          XMMRef t4(c->newVariable(VARIABLE_TYPE_XMM));
          XMMRef t5(c->newVariable(VARIABLE_TYPE_XMM));
          XMMRef t6(c->newVariable(VARIABLE_TYPE_XMM));
          XMMRef t7(c->newVariable(VARIABLE_TYPE_XMM));

          g->loadDQ(t0, dqword_ptr(src->c(), srcDisp +  0), srcAligned);
          g->loadDQ(t1, dqword_ptr(src->c(), srcDisp + 16), srcAligned);
          g->loadDQ(t2, dqword_ptr(src->c(), srcDisp + 32), srcAligned);
          g->loadDQ(t3, dqword_ptr(src->c(), srcDisp + 48), srcAligned);
          g->loadDQ(t4, dqword_ptr(src->c(), srcDisp + 64), srcAligned);
          g->loadDQ(t5, dqword_ptr(src->c(), srcDisp + 80), srcAligned);
          g->loadDQ(t6, dqword_ptr(src->c(), srcDisp + 96), srcAligned);
          g->loadDQ(t7, dqword_ptr(src->c(), srcDisp + 112), srcAligned);

          g->storeDQ(dqword_ptr(dst->c(), dstDisp +  0), t0, nt, dstAligned);
          g->storeDQ(dqword_ptr(dst->c(), dstDisp + 16), t1, nt, dstAligned);
          g->storeDQ(dqword_ptr(dst->c(), dstDisp + 32), t2, nt, dstAligned);
          g->storeDQ(dqword_ptr(dst->c(), dstDisp + 48), t3, nt, dstAligned);
          g->storeDQ(dqword_ptr(dst->c(), dstDisp + 64), t4, nt, dstAligned);
          g->storeDQ(dqword_ptr(dst->c(), dstDisp + 80), t5, nt, dstAligned);
          g->storeDQ(dqword_ptr(dst->c(), dstDisp + 96), t6, nt, dstAligned);
          g->storeDQ(dqword_ptr(dst->c(), dstDisp + 112), t7, nt, dstAligned);

          offset += 32;
          i -= 32;
        }
        else if (i >= 16 && c->numFreeXmm() >= 4)
        {
          XMMRef t0(c->newVariable(VARIABLE_TYPE_XMM));
          XMMRef t1(c->newVariable(VARIABLE_TYPE_XMM));
          XMMRef t2(c->newVariable(VARIABLE_TYPE_XMM));
          XMMRef t3(c->newVariable(VARIABLE_TYPE_XMM));

          g->loadDQ(t0, dqword_ptr(src->c(), srcDisp +  0), srcAligned);
          g->loadDQ(t1, dqword_ptr(src->c(), srcDisp + 16), srcAligned);
          g->loadDQ(t2, dqword_ptr(src->c(), srcDisp + 32), srcAligned);
          g->loadDQ(t3, dqword_ptr(src->c(), srcDisp + 48), srcAligned);

          g->storeDQ(dqword_ptr(dst->c(), dstDisp +  0), t0, nt, dstAligned);
          g->storeDQ(dqword_ptr(dst->c(), dstDisp + 16), t1, nt, dstAligned);
          g->storeDQ(dqword_ptr(dst->c(), dstDisp + 32), t2, nt, dstAligned);
          g->storeDQ(dqword_ptr(dst->c(), dstDisp + 48), t3, nt, dstAligned);

          offset += 16;
          i -= 16;
        }
        else if (i >= 8 && c->numFreeXmm() >= 2)
        {
          XMMRef t0(c->newVariable(VARIABLE_TYPE_XMM));
          XMMRef t1(c->newVariable(VARIABLE_TYPE_XMM));

          g->loadDQ(t0, dqword_ptr(src->c(), srcDisp +  0), srcAligned);
          g->loadDQ(t1, dqword_ptr(src->c(), srcDisp + 16), srcAligned);

          g->storeDQ(dqword_ptr(dst->c(), dstDisp +  0), t0, nt, dstAligned);
          g->storeDQ(dqword_ptr(dst->c(), dstDisp + 16), t1, nt, dstAligned);

          offset += 8;
          i -= 8;
        }
        else if (i >= 4)
        {
          XMMRef t0(c->newVariable(VARIABLE_TYPE_XMM));

          g->loadDQ(t0, dqword_ptr(src->c(), srcDisp), srcAligned);
          g->storeDQ(dqword_ptr(dst->c(), dstDisp), t0, nt, dstAligned);

          offset += 4;
          i -= 4;
        }
        else
        {
          SysIntRef t(c->newVariable(VARIABLE_TYPE_INT32));

          g->loadD(t, dword_ptr(src->c(), srcDisp));
          g->storeD(dword_ptr(dst->c(), dstDisp), t, nt);

          offset++;
          i--;
        }
      } while (i > 0);
      break;
    }
  }
}

} // BlitJit namespace
