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
#ifndef _BLITJITGENERATOR_H
#define _BLITJITGENERATOR_H

// [Dependencies]
#include "BlitJitConfig.h"

#include <AsmJit/AsmJitAssembler.h>
#include <AsmJit/AsmJitCompiler.h>

namespace BlitJit {

// [BlitJit - Forward declarations]
struct GeneratorOp;
struct GeneratorOpComposite32_SSE2;

// [BlitJit - Calling Convention]

#if defined(BLITJIT_X86)
# define BLITJIT_CALL ASMJIT_FASTCALL_2
enum { CConv = AsmJit::CALL_CONV_MSFASTCALL };
#else
# define BLITJIT_CALL
enum { CConv = AsmJit::CALL_CONV_DEFAULT };
#endif

//! @addtogroup BlitJit_Main
//! @{

typedef void (BLITJIT_CALL *FillSpanFn)(
  void* dst, const UInt32 src, SysUInt len);

typedef void (BLITJIT_CALL *BlitSpanFn)(
  void* dst, const void* src, SysUInt len);

typedef void (BLITJIT_CALL *BlitSpanMaskFn)(
  void* dst, const void* src, const void* mask, SysUInt len);

typedef void (BLITJIT_CALL *BlitRectFn)(
  void* dst, const void* src,
  SysInt dstStride, SysInt srcStride,
  SysUInt width, SysUInt height);

typedef void (BLITJIT_CALL *BlitRectMaskFn)(
  void* dst, const void* src, const void* mask,
  SysInt dstStride, SysInt srcStride, SysInt maskStride,
  SysUInt width, SysUInt height);

struct Generator
{
  // [Typedefs]

  typedef AsmJit::MMData MMData;
  typedef AsmJit::XMMData XMMData;

  typedef AsmJit::Register Register;
  typedef AsmJit::MMRegister MMRegister;
  typedef AsmJit::XMMRegister XMMRegister;

  typedef AsmJit::PtrRef PtrRef;
  typedef AsmJit::Int32Ref Int32Ref;
  typedef AsmJit::SysIntRef SysIntRef;
  typedef AsmJit::MMRef MMRef;
  typedef AsmJit::XMMRef XMMRef;

  // [Enums]

  //! @brief Basic optimization flags.
  enum Optimize
  {
    //! @Brief Optimize only for X86/X64, don't use MMX or SSE2
    OptimizeX86 = 0,
    //! @brief Optimize for MMX.
    OptimizeMMX = 1,
    //! @brief Optimize for SSE/SSE2/SSE3/...
    OptimizeSSE2 = 2
  };

  enum Body
  {
    BodyRConst = (1 << 0)
  };

  // [Construction / Destruction]

  Generator(AsmJit::Compiler* c);
  virtual ~Generator();

  // [FillSpan]

  //! @brief Generate fill span function.
  virtual void fillSpan(const PixelFormat& pfDst, const PixelFormat& pfSrc, const Operation& op);

  // [BlitSpan / BlitRect]

  //! @brief Generate blit span function.
  void blitSpan(const PixelFormat& pfDst, const PixelFormat& pdSrc, const Operation& op);
  //! @brief Generate blit rect function.
  void blitRect(const PixelFormat& pfDst, const PixelFormat& pdSrc, const Operation& op);

  //! @brief Generate MemCpy32 block.
  //! @internal
  void _MemCpy32(
    PtrRef& dst, PtrRef& src, SysIntRef& cnt);

  //! @brief Generate Composite32 SSE2 optimized block.
  //! @internal
  void _Composite32_SSE2(
    PtrRef& dst, PtrRef& src, SysIntRef& cnt,
    GeneratorOpComposite32_SSE2& c_op);

  // [Helpers]
  void x86MemSet32(AsmJit::PtrRef& dst, AsmJit::Int32Ref& src, AsmJit::SysIntRef& cnt);

  // variables in fomrat cN means constants, index is N - Constants[N]
  void xmmExpandAlpha_0000AAAA(const XMMRegister& r);
  void xmmExpandAlpha_AAAAAAAA(const XMMRegister& r);
  void xmmExpandAlpha_00001AAA(const XMMRegister& r, const XMMRegister& c1);
  void xmmExpandAlpha_1AAA1AAA(const XMMRegister& r, const XMMRegister& c1);

  void xmmLerp2(
    const XMMRegister& dst0, const XMMRegister& src0, const XMMRegister& alpha0);
  void xmmLerp4(
    const XMMRegister& dst0, const XMMRegister& src0, const XMMRegister& alpha0,
    const XMMRegister& dst1, const XMMRegister& src1, const XMMRegister& alpha1);

  void xmmComposite2();

  // [Code Generation]

  void initRConst();

  // [Constants]

  struct Constants
  {
    //XMMData Cx00000000000001000000000000000100; // [0]
    XMMData Cx00800080008000800080008000800080; // [1]
    XMMData Cx00FF00FF00FF00FF00FF00FF00FF00FF; // [2]
    //XMMData Cx000000000000FFFF000000000000FFFF; // [3]
    //XMMData CxFFFFFFFFFFFF0100FFFFFFFFFFFF0100; // [4]
    XMMData CxDemultiply[256];
  };

  static Constants* constants;
  static void initConstants();

  // [Members]

  //! @brief Assembler stream.
  AsmJit::Compiler* c;
  //! @brief Function.
  AsmJit::Function* f;
  //! @brief Cpu optimizations to use.
  UInt32 optimize;
  //! @brief Cpu features, see @c AsmJit::CpuInfo::Feature enumeration.
  UInt32 features;
  //! @brief Function body flags.
  UInt32 body;

  //! @brief Register that holds address for MMX/SSE constants.
  PtrRef rconst;

private:
  // disable copy
  inline Generator(const Generator& other);
  inline Generator& operator=(const Generator& other);
};

//! @}

} // BlitJit namespace

// [Guard]
#endif // _BLITJITGENERATOR_H
