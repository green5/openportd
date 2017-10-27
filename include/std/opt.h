#ifndef OPT_H_
#define OPT_H_

#include "std.h"
#include <getopt.h>

namespace STD_H { namespace opt {

//using std::vector;
//using std::string;

struct Opts
{
  const char *version, *program;
  vector<struct Opt> vec;
  Opts():version(0),program(0){}
  static Opts opt_;
};

struct Opt
{
  enum { NONE=no_argument, REQ=required_argument, OPT=optional_argument, SHORT, LONG };
  string name, value;
  int size;
  int type; // b=bool i=int s=char* S=string  
  void *data;
  const char *help;
  bool isset;
  option o_;
  Opt(int size_,const char *a):name(a),size(size_)
  {
    isset = 0;
  }
  static void fatal(const char *fmt,...)
  {
    va_list arg;
    va_start(arg,fmt);
    vfprintf(stderr,fmt,arg);
    fputc('\n',stderr);
    va_end(arg);
    exit(1);
  }
  static Opt* define_(const char *a,int type,void *data,const char *help)
  {
    if(type==0) fatal("bad option type for %s",a);
    if(a[0]=='-' && a[1]!='-') 
    {
      if(strlen(a)!=2) fatal("bad short option name %s",a);
      Opts::opt_.vec.push_back(Opt(Opt::SHORT,a+1));
    }
    else if(a[0]=='-' && a[1]=='-') 
    {
      Opts::opt_.vec.push_back(Opt(Opt::LONG,a+2));
    }
    else fatal("bad option name %s",a);
    Opt *ret = &Opts::opt_.vec.back();
    ret->type = type;
    ret->data = data;
    ret->help = help;
    ret->o_.name = 0;
    ret->o_.has_arg = NONE;
    ret->o_.flag = 0;
    ret->o_.val = 0;
    return ret;    
  }
  int need()
  {
    return o_.has_arg;
  }
  void setneed(int need)
  {
    o_.has_arg = need;
  }
  void set(const char *z)
  {
    isset = true;
    if(1)
    {
      value = z ? z : ""; /// set but value optional
      switch(type)
      {
        case 'b': *(bool*)data = true; break;
        case 's': *(char**)data = (char*)value.c_str(); break;
        case 'S': *(string*)data = value; break;
      }
    }
    if(z) 
    {
      value = z;
      switch(type)
      {
        case 'i': *(int*)data = atoi(z); break; /// not number
      }
    }
  }
  static string getshort()
  {
    string ret;
    for(auto &i:Opts::opt_.vec) if(i.size==SHORT)
    {
      ret += i.name[0];
      if(i.need()) ret += ':';
    }
    return ret;
  }
  static vector<option> getlong()
  {
    vector<option> ret;
    for(auto &i:Opts::opt_.vec) if(i.size==LONG)
    {
      i.o_.name = i.name.c_str();
      ret.push_back(i.o_);
    }
    return ret;  
  }
  static Opt* getlong(int index)
  {
    int i = 0;
    for(auto &o:Opts::opt_.vec) if(o.size==LONG)
    {
      if(i==index) return &o;
      i++;
    }
    return 0;
  }
  string usage()
  {
    string ret;
    bool is = name.size()==1; ///
    ret += is ? "-" : "--";
    ret += name;
    string arg("ARG");
    switch(type)
    {
     case 'i': arg = "INT"; break; 
     case 's': arg = "STR"; break; 
     case 'S': arg = "STR"; break; 
    }
    string eq(is?" ":"=");
    if(need()==Opt::REQ) ret += eq + arg;
    if(need()==Opt::OPT) ret += "[" + eq + arg + "]";
    return ret;
  }
  static option test_option(const char *a)
  {
    option o;
    o.name = a;
    o.has_arg = 0;
    o.flag = 0;
    o.val = 0;
    return o;
  }
};
#if 0
static bool isset(void *x)
{
  for(auto &i:Opts::opt_.vec) if(x==i.data) return i.isset;
  return false;
}
#endif
namespace Type
{
  template<typename T> int id(T &x) { return 0; }
  template<> int id(bool &x) { return 'b'; }
  template<> int id(int &x) { return 'i'; }
  template<> int id(char* &x) { return 's'; }
  template<> int id(string &x) { return 'S'; }
  template<> int id(const char* &x) { return 's'; }
};
template<typename T> static T define(const char* opt_, T &x, const char *help=0, T z=T())
{
  int t = Type::id(x);
  Opt *o = Opt::define_(opt_,t,(void*)&x,help);
  o->setneed(t=='b'?Opt::NONE:Opt::REQ);
  return z;
}
template<typename T> static T define(const char* opt_, const char* opt2_, T &x, const char *help=0, T z=T())
{
  int t = Type::id(x);
  Opt *o = Opt::define_(opt_,t,(void*)&x,help);
  o->setneed(t=='b'?Opt::NONE:Opt::REQ);
  return z;
}
template<typename T> static T define_opt(const char* opt_, T &x, const char *help=0, T z=T())
{
  int t = Type::id(x);
  Opt *o = Opt::define_(opt_,t,(void*)&x,help);
  o->setneed(Opt::OPT);
  return z;
}
//#ifdef MAIN_H
static void version_()
{
  FILE *fout = stdout;
  fprintf(fout,"%s version %s\n",Opts::opt_.program,Opts::opt_.version);
  exit(0);
}
static void usage()
{
  FILE *fout = stdout;
  fprintf(fout,"%s version %s\n",Opts::opt_.program,Opts::opt_.version);
  fprintf(fout,"usage: %s [options] ..., where options:\n",Opts::opt_.program);
  size_t w = 0;
  for(auto &o:Opts::opt_.vec) w = std::max(w,o.usage().size());
  for(auto &o:Opts::opt_.vec) if(o.size==Opt::SHORT) fprintf(fout,"%-*s : %s\n",(int)w,o.usage().c_str(),o.help);
  for(auto &o:Opts::opt_.vec) if(o.size==Opt::LONG) fprintf(fout,"%-*s : %s\n",(int)w,o.usage().c_str(),o.help);
  exit(1);
}
int main(const char *version,int &ac, char *av[])
{
  Opts::opt_.version = version;
  Opts::opt_.program = av[0];
  int nerror = 0;
  auto l = Opt::getlong();
  l.push_back(Opt::test_option("help"));
  l.push_back(Opt::test_option("version"));
  l.push_back(Opt::test_option(NULL));
  auto s = Opt::getshort();
  while(1)
  {
    int option_index = 0;
    int c = getopt_long(ac, av, s.c_str(), &l.front(), &option_index);
    if(c==-1) break;
    else if(c=='?') nerror++;
    else if(c==0)
    {
      if(option_index<(int)l.size())
      {
        if(!strcmp(l[option_index].name,"help")) usage();
        if(!strcmp(l[option_index].name,"version")) version_();
      }
      Opt *o = Opt::getlong(option_index);
      if(o) o->set(optarg);
    }
    else
    {
      for(auto &o:Opts::opt_.vec)
      {
        if(o.size==Opt::SHORT && o.name[0]==c) o.set(optarg);
      }
    }
  }
  int nc = 1;
  while(optind < ac)
  {
    av[nc++] = av[optind++];
  }
  av[ac=nc] = 0;
  return nerror ? -1 : 0;
}
int main(int &ac, char *av[])
{
  return main("1.0",ac,av);
}
Opts Opts::opt_;
//#endif

}} // namespace STD_H { namespace opt {

#endif
