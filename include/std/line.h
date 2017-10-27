#ifndef LINE_H_
#define LINE_H_ 

#include <sys/types.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <time.h>
#include <sys/stat.h>

#ifdef __GNUC__
#include <unistd.h>
#endif

#ifndef _DDK
#include <exception>
#include <stdexcept>
#endif

namespace STD_H
{
  size_t vformat(char *ret,size_t n,const char *fmt,va_list a);
  string vformat(const char *fmt,va_list a);
  string vformat(const char *fmt,va_list a);
  string format(const char *fmt,...);
  static inline const char *shortpath(const char *path)
  {
    const char *t = 0;
    if (t == 0) t = strrchr(path, '\\');
    if (t == 0) t = strrchr(path, '/');
    if (t && t[1]) path = t + 1;
    return path;
  }  
  static inline const char *shortfunc(const char *func)
  {
    auto n = strlen(func);
    return n < 40 ? func : "";
  }  
}

namespace STD_H
{
  struct Line
  {
    enum { EXIT_=0,ASSERT_=1,THROW_=2,FATAL_=3,ERR_=4,LOG_=5 };
    const char* file; int line; const char *func;
    unsigned info;
    const char *expr;
    bool errorSet; long errorCode;
    Line(const char* file_, int line_, const char *func_,int info_=LOG_, const char *expr_=0)
    {
      file = shortpath(file_);
      line = line_;
      func = shortfunc(func_);
      info = info_;
      expr = expr_;
      errorSet = false;
      errorCode = errno;
    }
    std::string str() const
    {
      char t[100];
      snprintf(t, sizeof(t), "%s.%d.%s",file,line,func);
      return t;
    }
    static void vlog(std::string &t)
    {
      t += "\n";
      (void)(::write(1,t.c_str(),t.size())==-1);
    }
    static void vlog(Line &v,const char *fmt, va_list a)
    {
      string t;
      if(v.info==ERR_) t += "Error: ";
      t += STD_H::format("%s.%d.%s: ",v.file,v.line,v.func);
      t += STD_H::vformat(fmt,a);
      if(v.errorSet) t += STD_H::format(" [%s]",strerror(v.errorCode));
#ifdef MYLOG
      MYLOG(t);
#else
      vlog(t);
#endif
      if(v.info==ASSERT_||v.info==THROW_) 
      {
        throw std::runtime_error(v.expr?v.expr:"bo");
      }
      if(v.info==EXIT_)
      {
        ::exit(v.errorCode);
      }
    }
    void plog_(const char *fmt = 0, ...)
    {
      va_list a; va_start(a, fmt); vlog(*this, fmt,a); va_end(a);
    }
    void plog_(long error,const char *fmt = 0, ...)
    {
      errorSet = true;
      errorCode = error;
      va_list a; va_start(a, fmt); vlog(*this, fmt,a); va_end(a);
    }
    bool passert_(const bool bo, const char *fmt=0, ...) //_Check_return_
    {
      if(!bo)
      {
        va_list a; va_start(a, fmt); vlog(*this, fmt,a); va_end(a);
      }
      return bo;
    }
  };
}

#ifndef __FLF__
#define __FLF__ __FILE__,__LINE__,__FUNCTION__ 
#define __Line__ STD_H::Line(__FLF__)
#endif

#define plog    STD_H::Line(__FLF__,STD_H::Line::LOG_).plog_
#define dlog    if(DEBUG) STD_H::Line(__FLF__,STD_H::Line::LOG_).plog_
#define perr    STD_H::Line(__FLF__,STD_H::Line::ERR_).plog_
#define pfatal  STD_H::Line(__FLF__,STD_H::Line::FATAL_).plog_
#define pexit   STD_H::Line(__FLF__,STD_H::Line::EXIT_).plog_
#define pthrow  STD_H::Line(__FLF__,STD_H::Line::THROW_).plog_
#define passert(bo,...) STD_H::Line(__FLF__,STD_H::Line::ASSERT_,#bo).passert_(bo,"" __VA_ARGS__)
#define pbool(...) pbool_(__FLF__,__VA_ARGS__)
#define pkernel  STD_H::Line::pkernel_

#define PLOG    plog("LOG")
#define PERR    perr(errno,"ERROR")
#define PEXIT   pexit(errno,"EXIT")
#define PTHROW  pthrow()
#define PFATAL  pfatal()
#define PKERNEL pkernel()
#define DLOG    dlog()
#define PBOOL   pbool_(__FLF__)
  
#endif
