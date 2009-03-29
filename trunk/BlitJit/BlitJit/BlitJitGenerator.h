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
#ifndef _BLITJITGENERATOR_H
#define _BLITJITGENERATOR_H

// [Dependencies]
#include <AsmJit/AsmJitAssembler.h>
#include <AsmJit/AsmJitCompiler.h>

#include "BlitJitBuild.h"

namespace BlitJit {

//! @addtogroup BlitJit_Main
//! @{

// forward declarations
struct GeneratorOp;
struct GeneratorOp_Composite32_SSE2;

// ============================================================================
// [BlitJit::Generator]
// ============================================================================

//! @brief Generator.
struct BLITJIT_API Generator
{
  // --------------------------------------------------------------------------
  // [AsmJit Shortcuts]
  // --------------------------------------------------------------------------

  // We are not using AsmJit namespace in header files, this is convenience for
  // code readibility. These types are used many times in Generator.

  typedef AsmJit::MMData MMData;
  typedef AsmJit::XMMData XMMData;

  typedef AsmJit::Register Register;
  typedef AsmJit::MMRegister MMRegister;
  typedef AsmJit::XMMRegister XMMRegister;

  typedef AsmJit::Mem Mem;

  typedef AsmJit::PtrRef PtrRef;
  typedef AsmJit::Int32Ref Int32Ref;
#if defined(ASMJIT_X64)
  typedef AsmJit::Int64Ref Int64Ref; 
#endif // ASMJIT_X64
  typedef AsmJit::SysIntRef SysIntRef;
  typedef AsmJit::MMRef MMRef;
  typedef AsmJit::XMMRef XMMRef;

  // --------------------------------------------------------------------------
  // [Optimization Flags]
  // --------------------------------------------------------------------------

  //! @brief Generator optimization flags.
  enum Optimize
  {
    //! @Brief Optimize only for X86/X64, don't use MMX or SSE2 registers
    //!
    //! If MMX/SSE2 is present and this flag is set, some special SSE2 
    //! extensions can be used to improve speed (mainly non-thermal hints).
    OptimizeX86 = 0,

    //! @brief Optimize for MMX.
    //!
    //! MMX optimization can use SSE or 3dNow instructions, but SSE registers
    //! will not be used.
    OptimizeMMX = 1,

    //! @brief Optimize for SSE/SSE2/SSE3/...
    //!
    //! This is maximum optimization that can be done by BlitJit. Generator can
    //! use MMX/SSE/SSE2 and newer instructions to generate best code. Also 
    //! this is most supported optimization.
    OptimizeSSE2 = 2
  };

  // --------------------------------------------------------------------------
  // [Body Flags]
  // --------------------------------------------------------------------------

  enum Body
  {
    BodyUsingConstants  = 0x01,
    BodyUsingMMZero     = 0x02,
    BodyUsingXMMZero    = 0x04
  };

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  //! @brief Create new @c Generator instance and assign Compiler @a c to it.
  //!
  //! Creating generator instance will generate no instructions, use @c gen...
  //! methods to do this job.
  Generator(AsmJit::Compiler* c);

  //! Destroy @c Generator instance.
  virtual ~Generator();

  // --------------------------------------------------------------------------
  // [FillSpan / FillRect]
  // --------------------------------------------------------------------------

  //! @brief Generate fill span function.
  void genFillSpan(const PixelFormat& pfDst, const PixelFormat& pfSrc, const Operation& op);

  //! @brief Generate fill rect function.
  void genFillRect(const PixelFormat& pfDst, const PixelFormat& pfSrc, const Operation& op);

  // --------------------------------------------------------------------------
  // [BlitSpan / BlitRect]
  // --------------------------------------------------------------------------

  //! @brief Generate blit span function.
  void genBlitSpan(const PixelFormat& pfDst, const PixelFormat& pdSrc, const Operation& op);

  //! @brief Generate blit rect function.
  void genBlitRect(const PixelFormat& pfDst, const PixelFormat& pdSrc, const Operation& op);

  // --------------------------------------------------------------------------
  // [Mov Helpers]
  // --------------------------------------------------------------------------

  //! @brief Emit streaming mov instruction
  //! (AsmJit::Serializer::mov() or AsmJit::Serializer::movnti() is used).
  void _StreamMov(const Mem& dst, const Register& src);
  //! @brief Emit streaming movq instruction
  //! (AsmJit::Serializer::movq() or AsmJit::Serializer::movntq() is used).
  void _StreamMovQ(const Mem& dst, const MMRegister& src);
  //! @brief Emit streaming movdqa instruction
  //! (AsmJit::Serializer::movdqa() or AsmJit::Serializer::movntdq() is used).
  void _StreamMovDQ(const Mem& dst, const XMMRegister& src);

  // --------------------------------------------------------------------------
  // [MemSet Helpers]
  // --------------------------------------------------------------------------

  //! @brief Generate MemSet32 block.
  //! @internal
  void _MemSet32(
    PtrRef& dst, Int32Ref& src, SysIntRef& cnt);

  // --------------------------------------------------------------------------
  // [MemCpy Helpers]
  // --------------------------------------------------------------------------

  //! @brief Generate MemCpy32 block.
  //! @internal
  void _MemCpy32(
    PtrRef& dst, PtrRef& src, SysIntRef& cnt);

  // --------------------------------------------------------------------------
  // [Composite Helpers]
  // --------------------------------------------------------------------------

  //! @brief Generate Composite32 SSE2 optimized block.
  //! @internal
  void _Composite32_SSE2(
    PtrRef& dst, PtrRef& src, SysIntRef& cnt,
    GeneratorOp_Composite32_SSE2& c_op);

  // --------------------------------------------------------------------------
  // [Constants Management]
  // --------------------------------------------------------------------------

  //! @brief Tell to generator that we are using constants, it may initialize
  //! base constants pointer in 64 bit mode.
  void usingConstants();

  //! @brief Make operand that contains constants address +/- custom 
  //! @a displacement.
  Mem getConstantsOperand(SysInt displacement = 0);

  // --------------------------------------------------------------------------
  // [MMX/SSE Zero Registers]
  // --------------------------------------------------------------------------

  void usingMMZero();
  void usingXMMZero();

  inline MMRef& mmZero() { return _mmZero; }
  inline XMMRef& xmmZero() { return _xmmZero; }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  //! @brief Assembler stream.
  AsmJit::Compiler* c;
  //! @brief Function.
  AsmJit::Function* f;
  //! @brief Cpu features, see @c AsmJit::CpuInfo::Feature enumeration.
  UInt32 features;
  //! @brief Cpu optimizations to use.
  UInt32 optimize;
  //! @brief Tells generator if it should use data prefetching.
  UInt32 prefetch;
  //! @brief Tells generator to use non-thermal hint for store (movntq, movntdq, movntdqa, ...)
  UInt32 nonThermalHint;
  //! @brief Function body flags.
  UInt32 body;

  //! @brief Register can contain address for MMX/SSE constants (see @c Api::Constants).
  //!
  //! @note Under 32 bit mode this register is not used (direct addressing is 
  //! used instead to save one register and improve performance).
  PtrRef _rConstantsAddress;

  //! @brief MMX zero register (used for unpacking).
  MMRef _mmZero;

  //! @brief SSE zero register (used for unpacking).
  XMMRef _xmmZero;

private:
  // disable copy
  BLITJIT_DISABLE_COPY(Generator);
};

//! @}

} // BlitJit namespace

// [Guard]
#endif // _BLITJITGENERATOR_H
