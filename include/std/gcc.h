#ifndef STD_GCC_H_
#define STD_GCC_H_

#ifdef __unix__
#include <netinet/ip.h>
#include <netdb.h>
#include <stdint.h>
#include <unistd.h>
#include <dirent.h>
#include <syscall.h>
#endif

#ifdef _WIN32
#include <windows.h>
#endif

#if defined(__GNUC__) && defined(__cplusplus)
#if 1 == 0 && __cplusplus < 201103L 
#warning "complile with -std=c++0x option"
#endif
#include <cxxabi.h>
#endif

typedef __int128 int128_t;

#define hton16(x) htons(x)
#define hton32(x) htonl(x)
//#define hton64(x)

#define no_inline __attribute__((noinline))
#define no_except _GLIBCXX_USE_NOEXCEPT
#if defined(__GNUC__) && defined(__cplusplus)
#define FORMAT_ATTRIBUTE_  __attribute__((__format__(__printf__,2,3))) // 1=this
#else
#define FORMAT_ATTRIBUTE_
#endif

#define _Check_return_ __attribute__((warn_unused_result))
//#define _Must_inspect_result_ __attribute__((error_unused_result))

#ifndef debugger
#define debugger asm("int $3")
#endif

#if STL_ == 1 && 1 == 0
inline void* operator new(size_t n, void* a)
{
  return a;
}
#define new_ptr(p) ::new(p) 
#endif

#include <pthread.h>

namespace STD_H
{
  struct KSPIN_LOCK_
  {
    // serial\serenum\log.c
    pthread_mutex_t lock_;
    KSPIN_LOCK_()
    {
      lock_ = PTHREAD_MUTEX_INITIALIZER;
    }
    void acquire()
    {
      pthread_mutex_lock(&lock_);
    }
    void inline lock()
    {
      acquire();
    }
    void release()
    {
      pthread_mutex_unlock(&lock_);
    }
    void inline unlock()
    {
      release();
    }
  };
};

#ifndef _MSC_VER
#define __CRTDECL
#endif

///#include "pool.h"

#endif
