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
#ifndef _BLITJIT_BLITJIT_H
#define _BLITJIT_BLITJIT_H

// [Documentation]

//! @mainpage
//!

//! @defgroup BlitJit_Api Public API.
//!
//! Contains small subset of API declared to be public and easy to use.

//! @defgroup BlitJit_Config Configuration.
//!
//! Contains macros that can be redefined to fit to any project.

//! @defgroup BlitJit_Private Private API.
//!
//! Contains API used internally by BlitJit library (most part of library).

//! @addtogroup BlitJit_Config
//! @{

//! @def BLITJIT_API
//! @brief Attribute that's added to classes that can be exported if BlitJit
//! is compiled as a dll library.

//! @def BLITJIT_MALLOC
//! @brief Function to call to allocate dynamic memory.

//! @def BLITJIT_REALLOC
//! @brief Function to call to reallocate dynamic memory.

//! @def BLITJIT_FREE
//! @brief Function to call to free dynamic memory.

//! @def BLITJIT_ILLEGAL
//! @brief Code that is execute if an one or more operands are invalid.

//! @def BLITJIT_ASSERT
//! @brief Assertion macro. Default implementation calls @c BLITJIT_ILLEGAL
//! if assert fails.

//! @}

//! @namespace BlitJit
//! @brief Main BlitJit library namespace.

// [Includes]

#include "Build.h"

namespace BlitJit {

//! @addtogroup BlitJit_Api
//! @{

// ============================================================================
// [BlitJit - Architecture]
// ============================================================================

// define it only if it's not defined. In some systems we can
// use -D command in compiler to bypass this autodetection.
#if !defined(BLITJIT_X86) && !defined(BLITJIT_X64)
# if defined(__x86_64__) || defined(__LP64) || defined(__IA64__) || \
     defined(_M_X64)     || defined(_WIN64) 
#  define BLITJIT_X64 // x86-64
# else
// _M_IX86, __INTEL__, __i386__
#  define BLITJIT_X86
# endif
#endif
  
// ============================================================================
// [BlitJit - Calling Convention]
// ============================================================================

//! @def BLITJIT_CALL
//! @brief Calling convention of functions generated by BlitJit

#if defined(BLITJIT_X86)
# if defined(__GNUC__)
#  define BLITJIT_FASTCALL   __attribute__((regparm(3)))
#  define BLITJIT_STDCALL    __attribute__((stdcall))
#  define BLITJIT_CDECL      __attribute__((cdecl))
# else
#  define BLITJIT_FASTCALL   __fastcall
#  define BLITJIT_STDCALL    __stdcall
#  define BLITJIT_CDECL      __cdecl
# endif
#else
# define BLITJIT_FASTCALL_2
# define BLITJIT_STDCALL
# define BLITJIT_CDECL
#endif // BLITJIT_X86

#if defined(BLITJIT_X86)
# if defined(BLITJIT_USE_FASTCALL)
#  define BLITJIT_CALL BLITJIT_FASTCALL
# endif
# if defined(BLITJIT_USE_STDCALL)
#  define BLITJIT_CALL BLITJIT_STDCALL
# endif
# if defined(BLITJIT_USE_CDECL)
#  define BLITJIT_CALL BLITJIT_CDECL
# endif
#endif

#if !defined(BLITJIT_CALL)
# define BLITJIT_CALL
#endif

// ============================================================================
// [BlitJit - Premultiply / Demultiply Function Prototypes]
// ============================================================================

//! @brief Premultiply function prototype.
typedef void (BLITJIT_CALL *PremultiplyFn)(
  void* dst, SysUInt len);

//! @brief Demultiply function prototype.
typedef void (BLITJIT_CALL *DemultiplyFn)(
  void* dst, SysUInt len);

// ============================================================================
// [BlitJit - Span Function Prototypes]
// ============================================================================

//! @brief Fill span function prototype.
typedef void (BLITJIT_CALL *FillSpanFn)(
  void* dst, const void* src, SysUInt len);

//! @brief Fill span function prototype (with closure).
typedef void (BLITJIT_CALL *FillSpanClosureFn)(
  void* dst, const void* src, SysUInt len, const void* closure);

//! @brief Blit span function prototype.
typedef void (BLITJIT_CALL *BlitSpanFn)(
  void* dst, const void* src, SysUInt len);

//! @brief Blit span function prototype (with closure).
typedef void (BLITJIT_CALL *BlitSpanClosureFn)(
  void* dst, const void* src, SysUInt len, const void* closure);

//! @brief Fill span with mask function prototype.
typedef void (BLITJIT_CALL *FillSpanMaskFn)(
  void* dst, const void* src, const void* msk, SysUInt len);

//! @brief Fill span with mask function prototype (with closure).
typedef void (BLITJIT_CALL *FillSpanMaskClosureFn)(
  void* dst, const void* src, const void* msk, SysUInt len, const void* closure);

//! @brief Fill span with mask function prototype.
typedef void (BLITJIT_CALL *BlitSpanMaskFn)(
  void* dst, const void* src, const void* msk, SysUInt len);

//! @brief Fill span with mask function prototype (with closure).
typedef void (BLITJIT_CALL *BlitSpanMaskClosureFn)(
  void* dst, const void* src, const void* msk, SysUInt len, const void* closure);

// ============================================================================
// [BlitJit - Rect Function Prototypes]
// ============================================================================

//! @brief Fill rect function prototype.
typedef void (BLITJIT_CALL *FillRectFn)(
  void* dst, const void* src,
  SysInt dstStride,
  SysUInt width, SysUInt height);

//! @brief Fill rect function prototype (with closure).
typedef void (BLITJIT_CALL *FillRectClosureFn)(
  void* dst, const void* src,
  SysInt dstStride,
  SysUInt width, SysUInt height,
  const void* closure);

//! @brief Blit rect function prototype.
typedef void (BLITJIT_CALL *BlitRectFn)(
  void* dst, const void* src,
  SysInt dstStride, SysInt srcStride,
  SysUInt width, SysUInt height);

//! @brief Blit rect function prototype (with closure).
typedef void (BLITJIT_CALL *BlitRectClosureFn)(
  void* dst, const void* src,
  SysInt dstStride, SysInt srcStride,
  SysUInt width, SysUInt height,
  const void* closure);

//! @brief Fill rect with mask function prototype.
typedef void (BLITJIT_CALL *FillRectMaskFn)(
  void* dst, const void* src, const void* msk,
  SysInt dstStride, SysInt mskStride,
  SysUInt width, SysUInt height);

//! @brief Fill rect with mask function prototype (with closure).
typedef void (BLITJIT_CALL *FillRectMaskClosureFn)(
  void* dst, const void* src, const void* msk,
  SysInt dstStride, SysInt mskStride,
  SysUInt width, SysUInt height,
  const void* closure);

//! @brief Blit rect with mask function prototype.
typedef void (BLITJIT_CALL *BlitRectMaskFn)(
  void* dst, const void* src, const void* msk,
  SysInt dstStride, SysInt srcStride, SysInt mskStride,
  SysUInt width, SysUInt height);

//! @brief Blit rect with mask function prototype (with closure).
typedef void (BLITJIT_CALL *BlitRectMaskClosureFn)(
  void* dst, const void* src, const void* msk,
  SysInt dstStride, SysInt srcStride, SysInt mskStride,
  SysUInt width, SysUInt height,
  const void* closure);

// ============================================================================
// [BlitJit - Pixel Format]
// ============================================================================

//! @brief Pixel format.
struct BLITJIT_HIDDEN PixelFormat
{
  inline const char* name() const { return _name; }
  inline UInt32 id() const { return _id; }

  inline UInt32 depth() const { return _depth; }
  inline UInt32 bytesPerPixel() const { return _depth >> 3; }

  inline UInt16 rMask16() const { return (((UInt16)1U << _rSize) - 1) << _rShift; }
  inline UInt16 gMask16() const { return (((UInt16)1U << _gSize) - 1) << _gShift; }
  inline UInt16 bMask16() const { return (((UInt16)1U << _bSize) - 1) << _bShift; }
  inline UInt16 aMask16() const { return (((UInt16)1U << _aSize) - 1) << _aShift; }

  inline UInt32 rMask32() const { return ((1U << _rSize) - 1) << _rShift; }
  inline UInt32 gMask32() const { return ((1U << _gSize) - 1) << _gShift; }
  inline UInt32 bMask32() const { return ((1U << _bSize) - 1) << _bShift; }
  inline UInt32 aMask32() const { return ((1U << _aSize) - 1) << _aShift; }

  inline UInt64 rMask64() const { return (((UInt64)1U << _rSize) - 1) << _rShift; }
  inline UInt64 gMask64() const { return (((UInt64)1U << _gSize) - 1) << _gShift; }
  inline UInt64 bMask64() const { return (((UInt64)1U << _bSize) - 1) << _bShift; }
  inline UInt64 aMask64() const { return (((UInt64)1U << _aSize) - 1) << _aShift; }

  inline UInt32 rShift() const { return _rShift; }
  inline UInt32 gShift() const { return _gShift; }
  inline UInt32 bShift() const { return _bShift; }
  inline UInt32 aShift() const { return _aShift; }

  inline UInt32 isPremultiplied() const { _isPremultiplied; }
  inline UInt32 isFloat() const { return _isFloat; }

  inline bool isArgb() const { return _rSize != 0 && _gSize != 0 && _bSize != 0 && _aSize != 0; }
  inline bool isAlpha() const { return _aSize != 0; }
  inline bool isGrey() const { return (_rShift == _gShift) & (_gShift == _bShift); }
  inline bool isRgb() const { return _rSize != 0 && _gSize != 0 && _bSize != 0; }

  //! @brief Pixel format IDs.
  enum Id
  {
    //! @brief 32 bit RGB format with alpha, colors are not premultiplied by alpha.
    ARGB32,
    //! @brief 32 bit RGB format with alpha, colors are premultiplied by alpha.
    PRGB32,
    //! @brief 32 bit RGB format without alpha. One byte is unused.
    XRGB32,

    //! @brief 24 bit RGB format in Big Endian Order.
    RGB24,
    //! @brief 24 bit RGB format in Little Endian Order.
    BGR24,

    //! @brief 8 bit alpha format used for masks.
    A8,

    //! @brief Count of formats.
    Count
  };

  // variables, not private, because this is structure usually read-only.

  //! @brief Pixel format name.
  char _name[32];
  //! @brief Pixel format id.
  UInt32 _id;

  //! @brief Pixel format depth.
  UInt32 _depth;

  //! @brief Red color size in bits.
  UInt32 _rSize;
  //! @brief Green color size in bits.
  UInt32 _gSize;
  //! @brief Blue color size in bits.
  UInt32 _bSize;
  //! @brief Alpha color size in bits.
  UInt32 _aSize;

  //! @brief Red color shift.
  UInt32 _rShift;
  //! @brief Green color shift.
  UInt32 _gShift;
  //! @brief Blue color shift.
  UInt32 _bShift;
  //! @brief Alpha color shift.
  UInt32 _aShift;

  //! @brief True if colors are premultiplied by alpha.
  UInt32 _isPremultiplied : 1;
  //! @brief True if colors are in float format.
  UInt32 _isFloat : 1;
};

static UInt32 getARGB32AlphaPos(const PixelFormat* pf)
{
  return pf->aShift() / 8;
}

// ============================================================================
// [BlitJit - Operator]
// ============================================================================

//! @brief Operator.
struct BLITJIT_HIDDEN Operator
{
  inline const char* name() const { return _name; }
  inline UInt32 id() const { return _id; }

  inline UInt8 srcPixelUsed() const { return _srcPixelUsed; }
  inline UInt8 dstPixelUsed() const { return _dstPixelUsed; }
  inline UInt8 srcAlphaUsed() const { return _srcAlphaUsed; }
  inline UInt8 dstAlphaUsed() const { return _dstAlphaUsed; }

  enum Id
  {
    //! @brief Source to dest (dest will be altered).
    //! 
    //! Dca' = Sca
    //! Da'  = Sa
    CompositeSrc,

    //! @brief Dest to source (source will be altered).
    //!
    //! Dca' = Dca
    //! Da'  = Da
    CompositeDest,

    //! @brief Source over dest.
    //!
    //! Dca' = Sca + Dca.(1 - Sa)
    //! Da'  = Sa + Da - Sa.Da
    CompositeOver,

    //! @brief Dest over source.
    //!
    //! Dca' = Dca + Sca.(1 - Da)
    //! Da'  = Da + Sa - Da.Sa
    CompositeOverReverse,

    //! @brief Source in dest.
    //!
    //! Dca' = Sca.Da
    //! Da'  = Sa.Da 
    CompositeIn,

    //! @brief Dest in source.
    //!
    //! Dca' = Dca.Sa
    //! Da'  = Da.Sa
    CompositeInReverse,

    //! @brief Source out dest.
    //!
    //! Dca' = Sca.(1 - Da)
    //! Da'  = Sa.(1 - Da) 
    CompositeOut,

    //! @brief Dest out source.
    //!
    //! Dca' = Dca.(1 - Sa) 
    //! Da'  = Da.(1 - Sa) 
    CompositeOutReverse,

    //! @brief Source atop dest.
    //!
    //! Dca' = Sca.Da + Dca.(1 - Sa)
    //! Da'  = Da
    CompositeAtop,

    //! @brief Dest atop source.
    //!
    //! Dca' = Dca.Sa + Sca.(1 - Da)
    //! Da'  = Sa 
    CompositeAtopReverse,

    //! @brief Source xor dest.
    //!
    //! Dca' = Sca.(1 - Da) + Dca.(1 - Sa)
    //! Da'  = Sa + Da - 2.Sa.Da 
    CompositeXor,

    //! @brief Clear to fully transparent or black (if image not contains alpha 
    //! channel).
    //!
    //! Dca' = 0
    //! Da'  = 0
    CompositeClear,

    //! @brief The source is added to the destination and replaces the 
    //! destination.
    //!
    //! Dca' = Sca + Dca
    //! Da'  = Sa + Da 
    CompositeAdd,

    //! @brief The source color is subtracted from destination and replaces 
    //! the destination.
    //!
    //! Dca' = Dca - Sca
    //! Da'  = 1 - (1 - Sa).(1 - Da)
    CompositeSubtract,

    //! @brief The source is multiplied by the destination and replaces 
    //! the destination.
    //!
    //! Dca' = Sca.Dca + Sca.(1 - Da) + Dca.(1 - Sa)
    //! Da'  = Sa + Da - Sa.Da 
    CompositeMultiply,

    //! @brief The source and destination are complemented and then multiplied 
    //! and then replace the destination.
    //! 
    //! Dca' = Sca + Dca - Sca.Dca
    //! Da'  = Sa + Da - Sa.Da 
    CompositeScreen,

    //! @brief Selects the darker of the destination and source colors.
    //!
    //! Dca' = min(Sca.Da, Dca.Sa) + Sca.(1 - Da) + Dca.(1 - Sa)
    //! Da'  = Sa + Da - Sa.Da 
    //!
    //! or 
    //!  
    //! if Sca.Da < Dca.Sa
    //!   src-over()
    //! otherwise
    //!   dst-over()
    CompositeDarken,

    //! @brief Selects the lighter of the destination and source colors.
    //!
    //! Dca' = max(Sca.Da, Dca.Sa) + Sca.(1 - Da) + Dca.(1 - Sa)
    //! Da'  = Sa + Da - Sa.Da 
    //!
    //! or 
    //!  
    //! if Sca.Da > Dca.Sa
    //!   src-over()
    //! otherwise
    //!   dst-over()
    CompositeLighten,

    //! @brief Subtracts the darker of the two constituent colors from 
    //! the lighter. Painting with white inverts the destination color. 
    //! Painting with black produces no change.
    //!
    //! Dca' = abs(Dca.Sa - Sca.Da) + Sca.(1 - Da) + Dca.(1 - Sa)
    //!      = Sca + Dca - 2.min(Sca.Da, Dca.Sa)
    //! Da'  = Sa + Da - Sa.Da 
    CompositeDifference,

    //! @brief Produces an effect similar to that of 'difference', but 
    //! appears as lower contrast. Painting with white inverts the 
    //! destination color. Painting with black produces no change.
    //!
    //! Dca' = (Sca.Da + Dca.Sa - 2.Sca.Dca) + Sca.(1 - Da) + Dca.(1 - Sa)
    //! Da'  = Sa + Da - Sa.Da 
    CompositeExclusion,

    //! @brief Inverts alpha channel of destination based on alpha channel of
    //! source.
    //!
    //! Dca' = (Da - Dca) * Sa + Dca.(1 - Sa)
    //! Da'  = Sa + Da - Sa.Da
    CompositeInvert,

    //! @brief Inverts alpha channel and colors of destination based on alpha channel of
    //! source.
    //!
    //! Dca' = (Da - Dca) * Sca + Dca.(1 - Sa)
    //! Da'  = Sa + Da - Sa.Da
    CompositeInvertRgb,

    //! @brief Count of operators.
    Count
  };

  // variables, not private, because this structure is usually read-only.

  //! @brief Operator name.
  char _name[32];
  //! @brief Operator id.
  UInt32 _id;

  UInt8 _srcPixelUsed;
  UInt8 _dstPixelUsed;

  UInt8 _srcAlphaUsed;
  UInt8 _dstAlphaUsed;
};

// ============================================================================
// [Optimization Flags]
// ============================================================================

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

// ============================================================================
// [BlitJit - Api]
// ============================================================================

struct BLITJIT_API Api
{
  // --------------------------------------------------------------------------
  // [Initialization / Deinitialization]
  // --------------------------------------------------------------------------

  static void init();

  // --------------------------------------------------------------------------
  // [Pixel Formats]
  // --------------------------------------------------------------------------

  static const PixelFormat pixelFormats[PixelFormat::Count];

  // --------------------------------------------------------------------------
  // [Operators]
  // --------------------------------------------------------------------------

  static const Operator operators[Operator::Count];

  // --------------------------------------------------------------------------
  // [Generator]
  // --------------------------------------------------------------------------

  //! @brief Generate pixel premultiply function.
  static PremultiplyFn genPremultiply(
    const PixelFormat* dstPf);

  //! @brief Generate pixel demultiply function.
  static DemultiplyFn genDemultiply(
    const PixelFormat* dstPf);

  //! @brief Generate fill span function.
  static FillSpanFn genFillSpan(
    const PixelFormat* dstPf,
    const PixelFormat* srcPf, 
    const Operator* op);

  //! @brief Generate fill span with mask function.
  static FillSpanMaskFn genFillSpanWithMask(
    const PixelFormat* dstPf,
    const PixelFormat* srcPf,
    const PixelFormat* mskPf,
    const Operator* op);

  //! @brief Generate fill rect function.
  static FillRectFn genFillRect(
    const PixelFormat* dstPf,
    const PixelFormat* srcPf,
    const Operator* op);

  //! @brief Generate fill rect with mask function.
  static FillRectMaskFn genFillRectWithMask(
    const PixelFormat* dstPf,
    const PixelFormat* srcPf,
    const PixelFormat* mskPf,
    const Operator* op);

  //! @brief Generate blit span function.
  static BlitSpanFn genBlitSpan(
    const PixelFormat* dstPf,
    const PixelFormat* srcPf,
    const Operator* op);

  //! @brief Generate blit rect function.
  static BlitRectFn genBlitRect(
    const PixelFormat* dstPf,
    const PixelFormat* srcPf,
    const Operator* op);

  //! @brief Free generated function.
  static void freeFunction(void* fn);
};

//! @}

} // BlitJit namespace

// [Guard]
#endif // _BLITJIT_BLITJIT_H
