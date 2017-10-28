#ifndef STR_H_
#define STR_H_ STD_H

#include <ctype.h>

#ifndef __STRING
#define __STRING(x) #x
#endif
#define CONCATS_(x,y) __STRING(x) y

#ifndef NL
#define NL std::endl
#endif

#define DEFINE_LSTR(T) friend ostream& operator<<(ostream &ret,const T &a) {return ret<<a.str();}

#include <arpa/inet.h>

namespace STR_H_ {

/// int gcd(const vector<int> &a); // math.hpp

#if 0
template<typename T>
...
vsplit(const T* first,const T* last,const vector<> &delim,unsigned int limit=0)
{
  typedef pair<const T*,const T*> ret_t;
}
#endif

typedef pair<vector<string>::iterator,vector<string>::iterator> vsplit_ret_t;

static vector<vsplit_ret_t> vsplit(vector<string>::iterator first, const vector<string>::iterator &last, const vector<string> &delim=vector<string>(), unsigned int limit=0)
{
  vector<vsplit_ret_t> ret;
  if(delim.size()) for(auto i=first;i!=last;++i)
  {
    bool br = delim.size() == 0;
    //for(auto &d:delim) MS2010
    for(auto d=delim.begin();d!=delim.end();d++)
    {
      if(d->compare(*i)==0) { br=true; break; }
    }
    if(br)
    {
      if(limit && (ret.size()+1)==limit) break;
      if(i!=first) ret.push_back(vsplit_ret_t(first,i));
      first = i + 1;
    }
  }
  if(first!=last) ret.push_back(vsplit_ret_t(first,last));
  return ret;
}

static size_t skip(const char *str,const vector<string> &delim,size_t *ndelim=0)
{
  int ret = 0;
  if(ndelim) *ndelim = 0;
  if(str) for(;*str;)
  {
    size_t n = 0;
    auto d = delim.begin();
    for(;d!=delim.end();++d)
    {
      if(memcmp(str,d->c_str(),d->size())==0) 
      {
        n = d->size(); // not empty delim
        if(ndelim) *ndelim += 1;
        break;
      }
    }
    if(n==0) break;
    ret += n;
    str += n;
  }
  return ret; 
}

static size_t nskip(const char *str,const vector<string> &delim)
{
  int ret = 0;
  if(str) for(;*str;)
  {
    size_t n = 0;
    auto d = delim.begin();
    for(;d!=delim.end();++d)
    {
      if(memcmp(str,d->c_str(),d->size())==0) 
      {
        n = d->size(); // not empty delim
        break;
      }
    }
    if(n!=0) break;
    ret++;    
    str++;
  }
  return ret;
}

static inline bool npush(vector<string> &ret,const string &s,unsigned int limit)
{
  if(limit==0||ret.size()<limit)
  {
    ret.push_back(s);
    return true;
  }
  ret.back().append(s);
  return false;
}

static vector<string> vsplit(const char *str,const vector<string> &delim,int limit=0)
{
  vector<string> ret;
  --limit;
  if(str) for(;*str;)
  {
    size_t d;
    size_t n1 = skip(str,delim,&d);
    str += n1;
    while(limit==-1||(int)ret.size()<limit)
    {
      if(d-->1) ret.push_back(string()); else break;
    }
    if(limit==-1||(int)ret.size()<limit)
    {
      size_t n2 = nskip(str,delim);
      if(n2>0) ret.push_back(string(str, n2));
      str += n2;
    }
    else
    {
      if(*str) ret.push_back(string(str));
      break;
    }
  }
  return ret;
}
static vector<string> vsplit(const string &str,const vector<string> &delim,int limit=0)
{
  return vsplit(str.c_str(),delim,limit);
}

static vector<string> split(const char *str,const char *delim="\n",bool empty=false,int limit=-1)
{
  vector<string> ret;
  if(str) for(;*str;)
  {
    size_t n1 = strspn(str, delim); 
    str += n1;
    if(limit==-1||(int)ret.size()<(limit-1))
    {
      size_t n2 = strcspn(str, delim);
      if(n2>0 || empty) ret.push_back(string(str, n2));
      str += n2;
    }
    else
    {
      if(*str || empty) ret.push_back(string(str)); // allow delim
      break;
    }
  }
  return ret;
}

#if 0
static inline vector<string> split(const string &str,const char *delim="\n",int limit=0)
{
  return split(str.c_str(),delim,limit);
}
#endif

static string join(const vector<string>::iterator &a,const vector<string>::iterator &b,const string &sep="\n")
{
  string ret;
  for(auto i=a;i!=b;i++) 
  {
    if(i!=a) ret += sep;
    ret += *i;
  }
  return ret;
}

static string join(const vector<string>::const_iterator &a,const vector<string>::const_iterator &b,const string &sep="\n")
{
  string ret;
  for(auto i=a;i!=b;i++) 
  {
    if(i!=a) ret += sep;
    ret += *i;
  }
  return ret;
}
static string join(const vector<const char*>::const_iterator &a,const vector<const char*>::const_iterator &b,const string &sep="\n")
{
  string ret;
  for(auto i=a;i!=b;i++) 
  {
    if(i!=a) ret += sep;
    ret += *i;
  }
  return ret;
}
static string join(const vector<string> &a,const string &sep="\n")
{
  return join(a.begin(),a.end(),sep);
}
static string join(const vector<const char*> &a,const string &sep="\n")
{
  return join(a.begin(),a.end(),sep);
}
static string resize(const string &a,size_t n,char c)
{
  if(a.size()==n) return a;
  string ret(a);
  ret.resize(n,c);
  return ret;
}
static string urlencode(const string &a)
{
  string ret;
  for(auto i=a.begin();i!=a.end();i++)
  {
    unsigned char c = *i;
    if(c==' ' || (!isalnum(c) && c!='-' && c!='_' && c!='.'))
    {
      static const char hex[] = "0123456789ABCDEF";
      char b[4];
      b[0] = '%';
      b[1] = hex[ c>>4 ];
      b[2] = hex[ c&0x0f ];
      b[3] = 0;
      ret.append(b);        
    }
    else
      ret.append(1,c);
  }
  return ret;
}
static uchar hex(char c,bool *ret=0)
{
  if(c>='0' && c<='9') return c-'0';
  if(c>='a' && c<='f') return c-'a' + 10;
  if(c>='A' && c<='F') return c-'A' + 10;
  if(ret) *ret = false;
  return 0; 
}
static uchar octal(char c,bool *ret=0)
{
  if(c>='0' && c<='7') return c-'0';
  if(ret) *ret = false;
  return 0; 
}
static string urldecode(const string &a)
{
  string ret;
  for(auto i=a.begin();i!=a.end();i++)
  {
    unsigned char c = *i;
    if(c>127)
      ret.append(1,'?');
    else if(c=='+')
      ret.append(1,' ');
    else if(c=='%')
    {
      unsigned char h0 = *++i;
      if(i==a.end()) continue;
      unsigned char h1 = *++i;
      if(i==a.end()) continue;
      ret.append(1L,(unsigned char)((hex(h0)<<4)|hex(h1)));
    }
    else
      ret.append(1,c);    
  }
  return ret;
}
static string substring(const string &s,string::size_type start,string::size_type end=string::npos)
{
  string::size_type len = end == string::npos ? string::npos : end > start ? end-start : 0;
  return s.substr(start,len);
}
static string replace(const string &s,const string &sub,const string &by)
{
  string ret;
  string::size_type i = 0, n = 0;
  while((n=s.find_first_of(sub,n))!=string::npos)
  {
    ret += substring(s,i,n);
    ret += by;
    i = n + sub.size();
    n = n + sub.size();   
  }
  ret += substring(s,i,n);
  return ret;
}
} // NS_(STR_H_)

#if 0
#include <string>
#include <vector>
#include <list>
#include <set>
#include <map>
#endif

namespace STR_H_ {
  //using pair;
  //using string;
  //using vector;
  //using map;
  //using multimap;
#if 0
  template<typename T> string str(const T& a)
  {
    ostringstream ret;
    ret << a;
    return ret.str();
  }
#endif
  static string str(int a,const char *fmt="%d")
  {
    return STD_H::format(fmt,a);
  }
  static string dump1(const void *t, size_t size)
  {
    string ret;
    const unsigned char *s = (const unsigned char*)t;
    for(unsigned i=0;i<size;i++)
    {
      unsigned c = s[i];
      ret += format((s[i]>=' '&&s[i]<='~')?"%c":"[%02x]",c);
    }
    return ret;
  }
  static string dump(const void *t, size_t size)
  {
    string ret;
    const unsigned char *s = (const unsigned char*)t;
    ret += format("%p[len=%ld=%lxh]:",s,size,size);
    if(size) ret += "\n";
    for(unsigned n=0;n<size;n+=16)
    {
      if(n) ret += "\n";
      ret += format("%04x  ",n);
      for(unsigned i=n;i<(n+16);i++)
      {
        ret += format(i<size?" %02x":"   ",s[i]);
      }
      ret += "  ";
      for(unsigned i=n;i<(n+16);i++)
      {
        ret += format(i<size?"%c":" ",(s[i]>=' '&&s[i]<='~')?s[i]:'.');
      }
    }
    return ret;
  }
  static string dump(const string &t)
  {
    return dump(t.c_str(),t.size());
  }
  static string dump1(const string &t)
  {
    return dump1(t.c_str(),t.size());
  }
  static string str(const void *t, size_t len)
  {
    string ret;
    for (unsigned i = 0; i < len; i++)
    {
      unsigned c = ((uchar*)t)[i];
      //ret += ::STD_H::format(c >= ' ' && c < 127 ? "%c" : "{%x}", c); //ret += ::STD_H::format(c >= ' ' && c < 127 && c!='\\' ? "%c" : "\\x{%02x}", c);
      ret += (char)c;
    }
    return ret;
  }
#if 1
  static string str(const string &a)
  {
    return str(a.c_str(),a.size());
  }
#else
  static string str(const string &a)
  {
    return a;
  }
#endif
#ifdef _POSIX_SOURCE___
  static string str2(const sockaddr &a,int flag='a') = delete
  {
    string host,port;
    const size_t alen=sizeof(sockaddr);
#ifdef NI_MAXHOST
    char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV]; // ws2tcpip.h
#endif
    if (getnameinfo(&a, alen, hbuf, sizeof(hbuf), sbuf, sizeof(sbuf), NI_NUMERICHOST | NI_NUMERICSERV) == 0)
    {
      host = hbuf;
      port = sbuf;      
    }
    return host;
  }
#endif
#if defined(AF_INET) && defined(AF_INET6)
  static sockaddr unstr(int af,const string &a_)
  {
    sockaddr ret;
    memset(&ret,0,sizeof(ret));
    string a(a_);
    char *h = (char*)a.c_str();
    char *p = strchr(h,':');
    if(p) *p++=0;
    bool isnum = h[0]>='0' && h[0]<='9';
    if (isnum && af == AF_INET)
    {
      sockaddr_in &i = (sockaddr_in&) ret;
      i.sin_family = af;
      inet_pton(af,h,&i.sin_addr);        
      i.sin_port = p?htons(atoi(p)):0;
    }
    else if (isnum && af == AF_INET6)
    {
      sockaddr_in6 &i = (sockaddr_in6&) ret;
      i.sin6_family = af;
      inet_pton(af,h,&i.sin6_addr);        
      i.sin6_port = p?htons(atoi(p)):0;
    }
    else
    {
      struct addrinfo hints;
      struct addrinfo *result = 0;
      memset(&hints, 0, sizeof(struct addrinfo));
      hints.ai_family = af ? af : AF_UNSPEC;
      if(getaddrinfo(h, p, &hints, &result)==0 && result)
      {
        ret = *result->ai_addr;
      }
      freeaddrinfo(result);
    }
    return ret; 
  }
  static string str(const sockaddr &a,int flag='a')
  {
    string host, port;
    const size_t alen=sizeof(sockaddr);
    if (a.sa_family == AF_INET)
    {
      const sockaddr_in &i = (const sockaddr_in&) a;
      const unsigned char *b = (const unsigned char*)&i.sin_addr;
      host = ::STD_H::format("%d.%d.%d.%d",b[0],b[1],b[2],b[3]);
      port = ::STD_H::format("%d",ntohs(i.sin_port));
    }
    else if (a.sa_family == AF_INET6)
    {
      const sockaddr_in6 &i = (const sockaddr_in6&)a;
      const unsigned char *b = (const unsigned char*)&i.sin6_addr;
      host = ::STD_H::format("%02X%02X:%02X%02X:%02X%02X:%02X%02X:%02X%02X:%02X%02X:%02X%02X:%02X%02X",b[0],b[1],b[2],b[3],b[4],b[5],b[6],b[7],b[8],b[9],b[10],b[11],b[12],b[13],b[14],b[15]);
      port = ::STD_H::format("%d",ntohs(i.sin6_port));
    }
    else
    {
      //host = ::STD_H::format("AF=%d,%s", a.sa_family,::STR_H_::str((void*)&a,alen).c_str());
      return ::STD_H::format("AF=%d", a.sa_family);
    }
    string ret;
    if(flag=='a') ret += host + ':' + port;
    else if(flag=='h') ret = host;
    else if(flag=='p') ret = port;
    return ret;
  }
  static string str(const sockaddr_in &a,int flag='a')
  {
    return str((const sockaddr&)a,flag);
  }
#endif
  template<typename T> string str(const vector<T> &a,const char *delim=",")
  {
#ifdef _GLIBCXX_SSTREAM
    ostringstream ret;
    for(unsigned int i=0;i<a.size();i++)
    {
      if(i) ret << delim;
      ret << str(a[i]);
    }
    return ret.str();
#else
    string ret;
    for(unsigned int i=0;i<a.size();i++)
    {
      if(i) ret += delim;
      ret += str(a[i]);
    }
    return ret;
#endif
  }
  string str(int x,const map<int,const char*> &a)
  {
    auto i = a.find(x);
    if(i!=a.end()) return i->second;
    return format("%d",x);
  }
#if 1 //auto
  template<typename T,typename V> string str(const map<T,V> &a,const char *delim=",")
  {
    string ret;
    int n = 0;
    for(const auto &i:a)
    {
      if(n++) ret += delim;
      string t;
      ret += str(i.first) + ":" + str(i.second) + ""; 
    }
    return ret;
  }
  template<typename T> string str(const set<T> &a,const char *delim=",")
  {
    string ret;
    int n = 0;
    for(const auto &i:a)
    {
      if(n++) ret += delim;
      ret += str(i);
    }
    return ret;
  }
#endif
#if 0
  static string str(const char* fmt, va_list a) //GCCBUG (&a remove)
  {
    char s[4000];
    if (fmt) vsnprintf(s, sizeof (s), fmt, a);
    return s;
  }
  static string vstr(const char* fmt, va_list a) //GCCBUG (&a remove)
  {
    char s[4000];
    if (fmt) vsnprintf(s, sizeof (s), fmt, a);
    return s;
  }
#endif
} //NS_(STR_H_)

namespace std { template<typename T1> ostream& operator<<(ostream& ret,const vector<T1> &a) { return ret << STR_H_::str(a); } }
namespace std { template<typename T1,typename T2> ostream& operator<<(ostream& ret,const map<T1,T2> &a) { return ret << STR_H_::str(a);} }

namespace STR_H_ {

static string bstring(const string &a)
{
  string t;
  for(size_t i=0;i<a.size();i++)
  {
    uchar c = a[i];
    if(c!='\\')
    {
      t += (char)c;
      continue;
    }
    uchar b = a[++i];
    switch(b)
    {
      case 'n': t += '\n'; break;
      case 't': t += '\t'; break;
      case 'v': t += '\v'; break;
      case 'b': t += '\b'; break;
      case 'r': t += '\r'; break;
      case 'f': t += '\f'; break;
      case 'a': t += '\a'; break;
      case '\\': t += '\\'; break;
      case '?': t += '\?'; break;
      case '\'': t += '\''; break;
      case '\"': t += '\"'; break;
      case '0': 
      {
        int o = 0;
        bool ok = true;
        for(; (c = octal(a[i], &ok)), ok; i++) o = (o << 3) | c;
        t += (uchar)(o);
        break;
      }
      case 'x': case 'X':
      {
        int h1 = hex(a[++i]);
        int h2 = hex(a[++i]);
        int hh = (h1 << 4) | h2;
        t += (uchar)(hh);
        break;
      }
      default: t += (char)b; break;
    }
  }
  return t;
}

static string cstring(const string &a)
{
  string t;
  for(size_t i=0;i<a.size();i++)
  {
    uchar c = a[i];
    switch(c)
    {
      case '\n': t += "\\n"; break;
      case '\t': t += "\\t"; break;
      case '\v': t += "\\v"; break;
      case '\b': t += "\\b"; break;
      case '\r': t += "\\r"; break;
      case '\f': t += "\\f"; break;
      case '\a': t += "\\a"; break;
      case '\\': t += "\\\\"; break;
      case '\?': t += "\\?"; break;
      case '\'': t += "\\'"; break;
      case '\"': t += "\\\""; break;
      default:
        if(1==1 && c>=' ' && c<127) 
        {
          t += (char)c;       
        }
        else
        {
          char q[10];
          STD_H::format(q,sizeof(q),"\\x%02x",c);
          t += q;
        }
        break;
    }
  }
  return t;
}

}

#if 0
namespace STD_H
{
  template<typename A> class Base
  {
    public:
    virtual string str() const
    {
      ostringstream ret;
      ret << *dynamic_cast<const A*>(this);
      return ret.str();
    }
  };
}
#endif

#endif
