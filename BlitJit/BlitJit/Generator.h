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
#ifndef _BLITJIT_GENERATOR_H
#define _BLITJIT_GENERATOR_H

// [Dependencies]
#include <AsmJit/Compiler.h>

#include "Build.h"

namespace BlitJit {

//! @addtogroup BlitJit_Generator
//! @{

// forward declarations
struct Module;
struct FillModule;
struct CompositeModule;

// ============================================================================
// [BlitJit::GeneratorBase]
// ============================================================================

struct BLITJIT_API GeneratorBase
{
  GeneratorBase(AsmJit::Compiler* c = NULL);
  virtual ~GeneratorBase();

  //! @brief Space for compiler if it wasn't passed to constructor.
  UInt8 _compiler[sizeof(AsmJit::Compiler)];

  //! @brief Compiler.
  AsmJit::Compiler* c;
  //! @brief Function.
  AsmJit::Function* f;

private:
  // disable copy
  BLITJIT_DISABLE_COPY(GeneratorBase);
};

// ============================================================================
// [BlitJit::Generator]
// ============================================================================

//! @brief Generator.
struct BLITJIT_API Generator : public GeneratorBase
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

  typedef AsmJit::Int32Ref Int32Ref;
#if defined(ASMJIT_X64)
  typedef AsmJit::Int64Ref Int64Ref; 
#endif // ASMJIT_X64
  typedef AsmJit::PtrRef PtrRef;
  typedef AsmJit::SysIntRef SysIntRef;
  typedef AsmJit::MMRef MMRef;
  typedef AsmJit::XMMRef XMMRef;

  // --------------------------------------------------------------------------
  // [Body Flags]
  // --------------------------------------------------------------------------

  enum Body
  {
    BodyUsingConstants  = 0x00000001,
    BodyUsingMMZero     = 0x00000010,
    BodyUsingXMMZero    = 0x00000100,
    BodyUsingXMM0080    = 0x00000200
  };

  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  //! @brief Create new @c Generator instance and assign Compiler @a c to it.
  //! @param c @c AsmJit::Compiler to attach to generator or NULL to tell
  //! generator to create own one.
  //!
  //! Creating generator instance will generate no instructions, use @c gen...
  //! methods to do this job.
  Generator(AsmJit::Compiler* c = NULL);

  //! Destroy @c Generator instance.
  virtual ~Generator();

  // --------------------------------------------------------------------------
  // [Getters / Setters]
  // --------------------------------------------------------------------------

  void setFeatures(UInt32 features);
  void setOptimization(UInt32 optimization);
  void setPrefetch(bool prefetch);
  void setNonThermalHint(bool nonThermalHint);

  inline UInt32 features() { return _features; }
  inline UInt32 optimization() { return _optimization; }
  inline bool prefetch() { return _prefetch; }
  inline bool nonThermalHint() { return _nonThermalHint; }

  // --------------------------------------------------------------------------
  // [Premultiply / Demultiply]
  // --------------------------------------------------------------------------

  //! @brief Generate pixel premultiply function.
  void genPremultiply(const PixelFormat& pfDst);

  //! @brief Generate pixel demultiply function.
  void genDemultiply(const PixelFormat& pfDst);

  // --------------------------------------------------------------------------
  // [FillSpan / FillRect]
  // --------------------------------------------------------------------------

  //! @brief Generate fill span function.
  void genFillSpan(const PixelFormat& pfDst, const PixelFormat& pfSrc, const Operator& op);

  //! @brief Generate fill rect function.
  void genFillRect(const PixelFormat& pfDst, const PixelFormat& pfSrc, const Operator& op);

  // --------------------------------------------------------------------------
  // [BlitSpan / BlitRect]
  // --------------------------------------------------------------------------

  //! @brief Generate blit span function.
  void genBlitSpan(const PixelFormat& pfDst, const PixelFormat& pdSrc, const Operator& op);

  //! @brief Generate blit rect function.
  void genBlitRect(const PixelFormat& pfDst, const PixelFormat& pdSrc, const Operator& op);

  // --------------------------------------------------------------------------
  // [Loops]
  // --------------------------------------------------------------------------

  struct Loop
  {
    bool finalizePointers;
  };

  void _GenFillLoop(
    PtrRef& dst,
    SysIntRef& cnt,
    FillModule* module,
    UInt32 kind,
    const Loop& loop);

  void _GenCompositeLoop(
    PtrRef& dst,
    PtrRef& src,
    SysIntRef& cnt,
    CompositeModule* module,
    UInt32 kind,
    const Loop& loop);

  // --------------------------------------------------------------------------
  // [Mov Helpers]
  // --------------------------------------------------------------------------

  void _LoadMov(const Register& dst, const Mem& src);
  void _LoadMovQ(const MMRegister& dst, const Mem& src);
  void _LoadMovDQ(const XMMRegister& dst, const Mem& src, bool aligned);

  //! @brief Emit streaming mov instruction
  //! (AsmJit::Serializer::mov() or AsmJit::Serializer::movnti() is used).
  void _StoreMov(const Mem& dst, const Register& src, bool nt);

  //! @brief Emit streaming movq instruction
  //! (AsmJit::Serializer::movq() or AsmJit::Serializer::movntq() is used).
  void _StoreMovQ(const Mem& dst, const MMRegister& src, bool nt);

  //! @brief Emit streaming movdqa instruction
  //! (AsmJit::Serializer::movdqa() or AsmJit::Serializer::movntdq() is used).
  //! 
  //! If @a aligned is false, AsmJit::Serializer::movdqu() instruction is used.
  void _StoreMovDQ(const Mem& dst, const XMMRegister& src, bool nt, bool aligned);

  // --------------------------------------------------------------------------
  // [Generator Helpers]
  // --------------------------------------------------------------------------

  //! @brief Extract alpha channel.
  //! @param dst0 Destination XMM register (can be same as @a src).
  //! @param src0 Source XMM register.
  //! @param packed Whether extract alpha for packed pixels (two pixels, 
  //! one extra instruction).
  //! @param alphaPos Alpha position.
  //! @param negate Whether to negate extracted alpha values (255 - alpha).
  void _ExtractAlpha(
    const XMMRegister& dst0, const XMMRegister& src0, UInt8 alphaPos0, UInt8 negate0, bool two);

  void _ExtractAlpha_4(
    const XMMRegister& dst0, const XMMRegister& src0, UInt8 alphaPos0, UInt8 negate0,
    const XMMRegister& dst1, const XMMRegister& src1, UInt8 alphaPos1, UInt8 negate1);

  // moveToT0 false:
  //   a0 = (a0 * b0) / 255, t0 is destroyed.
  // moveToT0 true:
  //   t0 = (a0 * b0) / 255, a0 is destroyed.
  void _PackedMultiply(
    const XMMRegister& a0, const XMMRegister& b0, const XMMRegister& t0,
    bool moveToT0 = false);

  void _PackedMultiplyWithAddition(
    const XMMRegister& a0, const XMMRegister& b0, const XMMRegister& t0);

  // moveToT0T1 false: 
  //   a0 = (a0 * b0) / 255, t0 is destroyed.
  //   a1 = (a1 * b1) / 255, t1 is destroyed.
  // moveToT0T1 true: 
  //   t0 = (a0 * b0) / 255, a0 is destroyed.
  //   t1 = (a1 * b1) / 255, a1 is destroyed.
  void _PackedMultiply_4(
    const XMMRegister& a0, const XMMRegister& b0, const XMMRegister& t0,
    const XMMRegister& a1, const XMMRegister& b1, const XMMRegister& t1,
    bool moveToT0T1 = false);

  // moveToT0 false:
  //   a0 = (a0 * b0 + c0 * d0) / 255, c0 and t0 are destroyed
  // moveToT0 true:
  //   t0 = (a0 * b0 + c0 * d0) / 255, a0 and c0 are destroyed
  void _PackedMultiplyAdd(
    const XMMRegister& a0, const XMMRegister& b0,
    const XMMRegister& c0, const XMMRegister& d0,
    const XMMRegister& t0, bool moveToT0 = false);

  void _PackedMultiplyAdd_4(
    const XMMRegister& a0, const XMMRegister& b0,
    const XMMRegister& c0, const XMMRegister& d0,
    const XMMRegister& t0,
    const XMMRegister& e0, const XMMRegister& f0,
    const XMMRegister& g0, const XMMRegister& h0,
    const XMMRegister& t1,
    bool moveToT0T1);

  void _Premultiply_1(
    const XMMRef& pix0);

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
  void usingXMM0080();

  inline MMRef& mmZero() { return _mmZero; }
  inline XMMRef& xmmZero() { return _xmmZero; }
  inline XMMRef& xmm0080() { return _xmm0080; }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  //! @brief Cpu features, see @c AsmJit::CpuInfo::Feature enumeration.
  UInt32 _features;
  //! @brief Cpu optimizations to use.
  UInt32 _optimization;
  //! @brief Tells generator if it should use data prefetching.
  bool _prefetch;
  //! @brief Tells generator to use non-thermal hint for store (movntq, movntdq, movntdqa, ...)
  bool _nonThermalHint;
  //! @brief Alignment of main loops.
  SysInt _mainLoopAlignment;
  //! @brief Function body flags.
  UInt32 _body;

  //! @brief Register can contain address for MMX/SSE constants (see @c Api::Constants).
  //!
  //! @note Under 32 bit mode this register is not used (direct addressing is 
  //! used instead to save one register and improve performance).
  PtrRef _rConstantsAddress;

  //! @brief MMX zero register (used for unpacking).
  MMRef _mmZero;

  //! @brief SSE zero register (used for unpacking).
  XMMRef _xmmZero;

  //! @brief SSE 0x0080 register (used for multiplication).
  XMMRef _xmm0080;
};

//! @}

} // BlitJit namespace

// [Guard]
#endif // _BLITJIT_GENERATOR_H
