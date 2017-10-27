#ifndef STL_H_
#define STL_H_

#if defined(__cplusplus) && STL_ == 0
#include <string>
#include <vector>
#include <list>
#include <set>
#include <map>
#include <functional>
#include <exception>
#include <algorithm>
#include <iostream>
#include <sstream>
#include <iterator>
#include <cstdio>
#include <cstdarg>
#include <assert.h>
#include <tuple>
using std::pair;
using std::tuple;
using std::string;
using std::wstring;
using std::vector;
using std::set;
using std::list;
using std::map;
using std::ostream;
using std::istream;
using std::ifstream;
using std::ofstream;
using std::fstream;
using std::stringbuf;
using std::stringstream;
using std::istringstream;
using std::ostringstream;
using std::cin;
using std::cout;
using std::cerr;
#include <thread>
//#include <sys/time.h>
#include <mutex>
#include <condition_variable>
#endif

#if defined(__cplusplus) && defined(STL_) && STL_ == 1
#define tstl std
#include "tstl/tstl_.h"
#include "tstl/chrono.h"
#include "tstl/allocator.h"
#include "tstl/char_traits.h"
#include "tstl/pair.h"
#include "tstl/vector.h"
#include "tstl/basic_string.h"
#include "tstl/set.h"
#include "tstl/map.h"
#include "tstl/list.h"
#include "tstl/function.h"
using std::function;
using std::allocator;
using std::pair;
using std::vector;
using std::map;
using std::set;
using std::list;
//using std::mutex;
//using std::unique_lock
//using std::condition_variable;
//using std::thread;
using std::wstring;
using std::string;
//template<class T, int N = -1> using nvector = vector<T, allocator<T, N>>;
//template<class T, int N = -1> using nstring = tstring<T, allocator<T, N>>;
class istream;
class ostream;
#endif

#if defined(__cplusplus) && defined(STL_) && STL_ == 2
#include "/home/green/src/cc/stl/ustl-2.0/upair.h"
#include "/home/green/src/cc/stl/ustl-2.0/uvector.h"
#include "/home/green/src/cc/stl/ustl-2.0/ustring.h"
#include "/home/green/src/cc/stl/ustl-2.0/list.h"
using ustl::pair;
using ustl::vector;
using ustl::string;
using ustl::list;
#endif

namespace STD_H
{
  template<typename T1, typename T2> static inline T1 min2(T1 t1, T2 t2)
  {
    return t1 <= t2 ? t1 : t2;
  }
  template<typename T1, typename T2> static inline T1 max2(T1 t1, T2 t2)
  {
    return t1 >= t2 ? t1 : t2;
  }
}

#endif // STD_H
