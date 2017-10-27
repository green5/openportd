// Copyright 2011 Rustem Valeev <r-green@mail.ru>
//
// std.h is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// std.h is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with std.h; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

#ifndef STD_H
#define STD_H my

typedef unsigned char uchar;
typedef unsigned char byte;

#include <string>
using std::string;

#ifndef vsizeof
#define vsizeof(v) (sizeof(v)/sizeof(v[0]))
#endif

#include "std/line.h"
#include "std/stl.h"

#ifdef __GNUC__
#include "std/gcc.h"
#endif

#ifdef _MSC_VER
#include "std/msc.h"
#endif

#include <exception>
namespace STD_H
{
  struct pexception : std::exception
  {
    string what_;
    pexception(const string &what):what_(what){}
    virtual ~pexception() throw() {}
    virtual const char* what() const /*no_except*/ throw() {return what_.c_str();}
  };
}

namespace STD_H
{
  size_t vformat(char *ret,size_t n,const char *fmt,va_list a)
  {
    size_t nret = vsnprintf(ret,n,fmt?fmt:"NULL",a);
    return nret;
  }
  string vformat(const char *fmt,va_list a)
  {
    string t;
    t.resize(4096);
    if(1==1)
    {
      size_t n = vformat((char*)t.c_str(),t.size(),fmt,a);
      t.resize(n);
    }
    return t;
  }
  string format(const char *fmt,...)
  {
    va_list a;
    va_start(a,fmt);
    string ret = vformat(fmt,a);
    va_end(a);
    return ret;
  }
}

#endif
