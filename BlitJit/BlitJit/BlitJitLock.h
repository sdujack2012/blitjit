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
#ifndef _BLITJITLOCK_H
#define _BLITJITLOCK_H

// [Dependencies]
#include "BlitJitConfig.h"

#if BLITJIT_OS == BLITJIT_WINDOWS
#include <windows.h>
#endif // BLITJIT_WINDOWS

#if BLITJIT_OS == BLITJIT_POSIX
#include <pthread.h>
#endif // BLITJIT_POSIX

namespace BlitJit {

//! @addtogroup BlitJit_Platform
//! @{

//! @brief Lock.
struct Lock
{
#if BLITJIT_OS == BLITJIT_WINDOWS
  typedef CRITICAL_SECTION Handle;
#endif // BLITJIT_WINDOWS
#if BLITJIT_OS == BLITJIT_POSIX
  typedef pthread_mutex_t Handle;
#endif // BLITJIT_POSIX

  inline Lock()
  {
#if BLITJIT_OS == BLITJIT_WINDOWS
    InitializeCriticalSection(&_handle);
    // InitializeLockAndSpinCount(&_handle, 2000);
#endif // BLITJIT_WINDOWS
#if BLITJIT_OS == BLITJIT_POSIX
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&_handle, &attr);
#endif // BLITJIT_POSIX
  }

  inline ~Lock()
  {
#if BLITJIT_OS == BLITJIT_WINDOWS
    DeleteCriticalSection(&_handle);
#endif // BLITJIT_WINDOWS
#if BLITJIT_OS == BLITJIT_POSIX
    pthread_mutex_destroy(&_handle);
#endif // BLITJIT_POSIX
  }

  inline Handle& handle()
  {
    return _handle;
  }

  inline const Handle& handle() const
  {
    return _handle;
  }

  inline void lock()
  { 
#if BLITJIT_OS == BLITJIT_WINDOWS
    EnterCriticalSection(&_handle);
#endif // BLITJIT_WINDOWS
#if BLITJIT_OS == BLITJIT_POSIX
    pthread_mutex_lock(&_handle);
#endif // BLITJIT_POSIX
  }

  inline void unlock()
  {
#if BLITJIT_OS == BLITJIT_WINDOWS
    LeaveCriticalSection(&_handle);
#endif // BLITJIT_WINDOWS
#if BLITJIT_OS == BLITJIT_POSIX
    pthread_mutex_unlock(&_handle);
#endif // BLITJIT_POSIX
  }

private:
  Handle _handle;

  // disable copy
  inline Lock(const Lock& other);
  inline Lock& operator=(const Lock& other);
};

struct AutoLock
{
  //! @brief Locks @a target.
  inline AutoLock(Lock& target) : _target(target)
  {
    _target.lock();
  }

  //! @brief Unlocks target.
  inline ~AutoLock()
  {
    _target.unlock();
  }

private:
  //! @brief Pointer to target (lock).
  Lock& _target;

  // disable copy
  inline AutoLock(const AutoLock& other);
  inline AutoLock& operator=(const AutoLock& other);
};

//! @}

} // BlitJit namespace

// [Guard]
#endif // _BLITJITLOCK_H
