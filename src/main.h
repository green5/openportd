#ifndef MAIN_H_
#define MAIN_H_

#define VERSION 1

#include <string>
void mylog(std::string&);
#define MYLOG mylog

#include "std.h"
#include "std/str.h"

void setport(struct sockaddr&,int port);
int getport(struct sockaddr &a);

#define C_STR str().c_str
#define PCHAR(c) ((c<=' '||c>=127)?'.':c)
extern int DEBUG;
static const string null;
extern string ext_ip;
extern int sosync;

typedef std::map<string,string> map_t;
using STD_H::format;
using STD_H::str;
using STD_H::unstr;
using STD_H::dump;
using STD_H::Line;

#ifdef USE_LEV
#include "lev.h"
using namespace lev;
#endif

#ifdef USE_EV
#include <ev++.h>

struct Socket
{
  int fd;
  Socket(int fd_):fd(fd_){}
  string local(int flag='a')
  {
    struct sockaddr name;
    socklen_t len = sizeof(name);
    if(getsockname(fd,&name,&len) == 0) return str(name,flag);
    return string();
  }
  string remote(int flag='a')
  {
    struct sockaddr name;
    socklen_t len = sizeof(name);
    if(getpeername(fd,&name,&len) == 0) return str(name,flag);
    return string();
  }
#define NAME(fd) Socket(fd).name(3).c_str()
  string name(int which)
  {
    string ret;
    if(which&1) ret += local();
    if(which&2) ret += "," + remote();
    return format("[%d,%s]",fd,ret.c_str());
  }
  int get(int name)
  {
    int ret = -1;
    socklen_t n;
    n = sizeof(ret);
    if(getsockopt(fd,SOL_SOCKET,name,&ret,&n)==-1) PERR;
    return ret;
  }
  void set(int name,int val)
  {
    if(setsockopt(fd,SOL_SOCKET,name,&val,sizeof(val))==-1) PERR;
  }
};

typedef struct EvSocket TSocket;
struct EvSocket
{
  struct Parent
  {
    virtual void onerror(int fd,const Line &line)
    {
      plog(errno,"%s fd=%s",line.C_STR(),NAME(fd));
      //EvSocket::loop("break",__Line__);
    }
    virtual int onlisten(int fd,sockaddr &addr)
    {
      plog("fd=%s %s",NAME(fd),str(addr).c_str());
      return -1;
    }
    virtual void onread(int fd) 
    {
      string buf;
      EvSocket::recv(fd, buf);
      if(buf.size()==0) 
      {
        plog("EOF");
        onerror(fd,__Line__);
      }
      else  
        plog("%ld",buf.size());
    }
    virtual void onwrite(int fd) 
    {
      plog("fd=%s",NAME(fd));
    }
  };
  ev::io io;
  Parent *parent;
  EvSocket(Parent *p):parent(p)
  {
    io.fd = -1;
  }
  virtual ~EvSocket()
  {
    //plog("close %d=%s",io.fd,NAME(io.fd));
    close(io.fd);
  }
  int fd() const
  {
    return io.fd;
  }
  void on_listen_socket(ev::io &watcher, int revents)
  {
    if(revents&EV_ERROR)
    {
      parent->onerror(watcher.fd,__Line__);
      revents &= ~EV_ERROR;
    }
    if(revents&EV_READ) 
    {
      struct sockaddr addr;
      socklen_t len = sizeof(addr);
      int fd = accept(watcher.fd, &addr, &len);
      if(fd == -1) return parent->onerror(watcher.fd,__Line__);
      if(parent->onlisten(fd,addr)==-1) close(fd); // access denied
      revents &= ~EV_READ;
    }
    if(revents) plog("revents=%xh",revents);
  }
  void on_connect_socket(ev::io &watcher, int revents)
  {
    //plog("revents=%xh",revents);
    if(revents&EV_ERROR) 
    {
      parent->onerror(watcher.fd,__Line__);
      revents &= ~EV_ERROR;
    }
    if(revents&EV_READ) 
    {
      parent->onread(watcher.fd);
      revents &= ~EV_READ;
    }
    if(revents&EV_WRITE) 
    {
      parent->onwrite(watcher.fd);
      revents &= ~EV_WRITE;
    }
    if(revents) plog("revents=%xh",revents);
  }
  static int socket_(struct sockaddr *a)
  {
    int s = ::socket(a->sa_family, SOCK_STREAM, 0);
    if(s==-1) PEXIT;
    return s;
  }
  virtual void setoption(int fd)
  {
    if(!sosync && fcntl(fd,F_SETFL,fcntl(fd,F_GETFL,0)|O_NONBLOCK)==-1) PERR; 
  }
  EvSocket& listen(sockaddr &addr,int backlog)
  {
    int s = socket_(&addr);
    int opt = 1;
    setoption(s);
    if(setsockopt(s,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt))==-1) PERR;
    if(bind(s,&addr,sizeof(addr))==-1)
    {
      plog(errno,"%s",str(addr).c_str());
      loop("break",__Line__);
    }
    socklen_t len = sizeof(addr);
    (void)getsockname(s,&addr,&len); // update addr
    dlog("listen on fd=%s",NAME(s));
    ::listen(s,backlog);
    io.set<EvSocket,&EvSocket::on_listen_socket>(this);
    io.start(s,ev::READ);
    return *this;
  }
  EvSocket& listen(const string &port,int backlog=5)
  {
    struct sockaddr addr = unstr(AF_INET,port);
    return listen(addr,backlog);
  }
  EvSocket& connect(int s)
  {
    setoption(s);
    io.set<EvSocket,&EvSocket::on_connect_socket>(this);
    io.start(s,ev::READ);
    return *this;
  }
  EvSocket& connect(const string &port,const string &sport=string())
  {
    struct sockaddr addr = unstr(AF_INET,port);
    int s = socket_(&addr); 
    if(sport.size())
    {
      struct sockaddr addr = unstr(AF_INET,sport);
      if(bind(s,&addr,sizeof(addr))==-1)
      {
        pexit(errno,"%s",str(addr).c_str());
      }
    }
    if(::connect(s,&addr,sizeof(addr))==-1) 
    {
      //if(!(errno==EINPROGRESS)) 
      perr(errno,"%s",str(addr).c_str());
      parent->onerror(s,__Line__);
      close(s);
      return *this;
    }
    setoption(s);    
    io.set<EvSocket,&EvSocket::on_connect_socket>(this);
    io.start(s,ev::READ);
    return *this;
  }
  static ev::default_loop loop_;
  static void loop(const string &cmd,const Line &line)
  {
    dlog("%s from %s",cmd.c_str(),line.C_STR());
    if(cmd=="exit") exit(1);
    if(cmd=="run") loop_.run(0);
    if(cmd=="break") loop_.break_loop();
  }  
  static size_t recv(int fd,string &ret)
  {
    size_t nret = 0;
    for (ssize_t x = ret.size();;)
    {
      const ssize_t nn = 4 * 1024;
      ret.resize(ret.size() + nn);
      ssize_t n = ::recv(fd, (char*) ret.c_str() + x, nn, MSG_DONTWAIT);
      if (n == -1) n = 0;
      if (nn - n) ret.resize(ret.size() - nn + n);
      nret += n;
      if (n == 0 || n < nn) break;
      x += n;
    }
    return nret;
  }
  void write(const string &a)
  {
    ssize_t n = ::write(io.fd,a.c_str(),a.size());
    if(n!=a.size()) 
    {
      perr(errno,"n=%ld a=%ld",n,a.size());
      parent->onerror(io.fd,__Line__);
    }
  }
  void printf(const char *fmt,...)
  {
    va_list a;
    va_start(a, fmt); 
    string t = format(fmt,a); 
    va_end(a);
    write(t);
  }
  string local(int flag='a')
  {
    return Socket(fd()).local(flag);
  }
  string remote(int flag='a')
  {
    return Socket(fd()).remote(flag);
  }
};
ev::default_loop EvSocket::loop_;
#endif

#include "config.h"

typedef int tid_t; 

struct Header
{
  unsigned size;
  char type,sign;
  tid_t fr,to;
  string str() const
  {
    return format("size=%d (%c) #%d->#%d",size,type,fr,to);
  }
};

#include <sys/uio.h>

struct Packet 
{
  struct Header head;
  string data;
  static int unpack(string &buf,Packet &packet)
  {
    if(buf.size()<sizeof(head)) return 0;
    packet.head = *(Header*)buf.c_str();
    if(packet.head.sign!='p') return -1;
    if(buf.size()<packet.head.size) return 0;
    packet.data = buf.substr(sizeof(head),packet.head.size-sizeof(head));
    buf = buf.substr(packet.head.size); // fixs, optimize later
    return 1;
  }
  string str(int v=0) const
  {
    if(v==0) return format("%s data=%ld+%ld",head.C_STR(),sizeof(head),data.size());
    return format("%s data=%ld+%ld%s",head.C_STR(),sizeof(head),data.size(),dump(data).c_str());
  }
  template<typename T> T& cast()
  {
    if(data.size()<sizeof(T)) pexit("data=%ld T=%ld",data.size(),sizeof(T));
    return *(T*)data.c_str();
  }
  struct b
  {
    int version;
    int port;
    char pw[16];
    b(int port_,const string &pw_):version(VERSION),port(port_)
    {
      memset(pw,0,sizeof(pw));
      strncpy(pw,pw_.c_str(),sizeof(pw)-1);
    }
  };
  struct c
  {
    tid_t addr;
    int port;
    c(tid_t addr_,int port_=0):addr(addr_),port(port_){}
  };
};

template<typename P> struct TChannel : TSocket, TSocket::Parent
{
  P *parent;
  TChannel(P*p):TSocket(this),parent(p)
  {
  }
  virtual void setoption(int fd)
  {
    TSocket::setoption(fd);
    int r1 = Socket(fd).get(SO_RCVBUF);
    int s1 = Socket(fd).get(SO_SNDBUF);
    Socket(fd).set(SO_SNDBUF,1000000);
    Socket(fd).set(SO_RCVBUF,1000000);
    int r2 = Socket(fd).get(SO_RCVBUF);
    int s2 = Socket(fd).get(SO_SNDBUF);
    dlog("rs_buf(%s) %d,%d -> %d,%d",NAME(fd),r1,s1,r2,s2);
  }
  void send_(tid_t fr,tid_t to,int type,const void *data=0,int size=0)
  {
    Header head;
    head.size = sizeof(head) + size;
    head.type = type;
    head.sign = 'p';
    head.fr = fr;
    head.to = to;
    if(DEBUG==1) plog("[%s] size=%d",head.C_STR(),size);
    if(DEBUG==2) plog("[%s] size=%d %s",head.C_STR(),size,dump(data,size).c_str());
    struct iovec io[2];
    io[0].iov_base = &head;
    io[0].iov_len = sizeof(head);
    io[1].iov_base = (void*)data;
    io[1].iov_len = data ? size : 0;
    ssize_t n = writev(fd(),io,data?2:1);
    if(n!=head.size) 
    {
      plog(errno,"fd=%s n=%ld v=%ld",NAME(fd()),n,sizeof(head)+size);
    }
  }
  void send(tid_t tid,int type,const string &data)
  {
    send_(0,tid,type,data.c_str(),data.size());
  }
  template<typename D> void send(tid_t tid,int type,const D& d)
  {
    send_(0,tid,type,&d,sizeof(d));
  }
  template<typename D,typename R> void call(int type,const D& d,R *ret)
  {
    send_(ret->tid,0,type,&d,sizeof(d));
  }
  template<typename D> void reply(const Packet &o,const D& d)
  {
    send_(0,o.head.fr,'R',&d,sizeof(d));
  }
  std::list<Packet*> write_queue;
  std::string read_buffer;
  virtual void onwrite(int fd)
  {
    io.set(write_queue.empty()?ev::READ:ev::READ|ev::WRITE);
  }
  void read_(int fd,Packet &packet)
  {
    if(DEBUG==1) plog("[%s]",packet.C_STR());
    if(DEBUG==2) plog("[%s]%s",packet.C_STR(),dump(packet.data).c_str());
    switch(packet.head.type)
    {
      case 'L':
      {
        plog("%s",packet.data.c_str());
        break;
      }
      case 'E':
      {
        perr("%s",packet.data.c_str());
        parent->finish(__Line__,"exit"); /// server?
        break;
      }
      case 'R':
      {
        auto t = parent->map_.find(packet.head.to);
        if(t) t->onreply(packet);
        else plog("fd=%s unknown reply %s",NAME(this->fd()),packet.C_STR());
        break;
      }
      default:
      {
        parent->onrpc(fd,packet);
        break;
      }
    }
  }
  virtual void onread(int fd)
  {
    size_t n = TSocket::recv(fd, read_buffer);
    if(n==0)
    {
      //if(errno==ENOTCONN) errno = 0;
      //if(errno==EINPROGRESS) errno = 0;
      //if(errno==ECHILD) errno = 0;
      if(errno) plog(errno,"fd=%s",NAME(fd));
      parent->finish(__Line__);
      return;
    }
    Packet packet;
    int x;
    while((x=Packet::unpack(read_buffer,packet))==1) read_(fd,packet);
    if(x==-1)
    {
      plog("fd=%s bad sign",NAME(fd));
      parent->finish(__Line__);
    }
  }
  virtual void onerror(int fd,const Line &line)
  {
    dlog("%s: rpc error fd=%s",line.C_STR(),NAME(fd));
    parent->finish(__Line__);
  }
};

#include <bsd/string.h>

struct http
{
  map_t head;
  string tail;
  string get(const char *a)
  {
    auto i = head.find(a);
    return i == head.end() ? "" : i->second;
  }
  void set(const char *a,const char *b)
  {
    head[a] = b;
  }  
  int parse(const string &data)
  {
    if(data.size()==0)
    {
      return -1;
    }
    char *t, *s, *e;
    s = (char*)data.c_str();
    e = s + data.size();
    for(int i=0;s<e;i++,s=t+2)
    {
      t = strnstr(s,"\r\n",e-s); //t = strstr(s,"\r\n");
      if(t==0) return -1;
      if(t==s) { t += 2; break; }
      if(i==0)
      {
        head["method"] = string(s,0,t-s);
      }
      else
      {
        char *c = strnstr(s,": ",t-s);
        if(c==0) return -1;
        else head[string(s,c-s)] = string(c+2,t-c-2);
      }
    }
    s = (char*)data.c_str();
    size_t hlen = t - s;
    size_t tlen = s + data.size() - t;
    tail = string(t,tlen);
    if(tlen==0) return 1; // done, only headers
    if(head.find("Content-Encoding")!=head.end() && head["Content-Encoding"]=="gzip")
    {
      // Content-Encoding: gzip (size[4] + "\n" + h[8] + ...)
      size_t nz = strtol(string(t,4).c_str(),NULL,16) + 4 + 1 + 8;
      return nz <= tlen ? 1 : 0;
    }
    if(head.find("Content-Length")!=head.end())
    {
      // tail[Content-Length]
      size_t nz = strtol(head["Content-Length"].c_str(),NULL,10);
      return nz <= tlen ? 1 : 0;
    }
    plog("data=%ld tail=%d",data.size(),tlen);
    return 0;
  }
  string data() const
  {
    string ret;
    string eos = "\r\n";
    auto i = head.find("method");
    if(i==head.end())
    {
      PLOG;
      return ret;
    }
    ret += i->second + eos;
    for(auto &i:head) 
    {
      ret += i.first + ": " + i.second + eos;
    }    
    ret += eos;    
    ret += tail;
    return ret;
  }
  string str() const
  {
    string ret;
    for(auto &i:head) ret += i.first + ": " + i.second + "\n";
    ret += format("tail: %ld,%ld",head.size(),tail.size());
    return ret;
  }
  void log(const string &h) const
  {
    for(auto &i:head) plog("%s %s: %s",h.c_str(),i.first.c_str(),i.second.c_str());
    plog("%s tail: %ld,%ld",h.c_str(),head.size(),tail.size());
  }
};

template<typename T> struct Map
{
  tid_t tid_;
  map<tid_t,T*> map_;
  Map():tid_(0)
  {
  }
  T *find(tid_t id)
  {
    auto i = map_.find(id);
    return i==map_.end() ? 0 : i->second;
  }
  void erase(T *t)
  {
    map_.erase(t->tid);
    delete t;
  }
  template<typename... A> T *create(A... a)
  {
    T *t = new T(++tid_ ? tid_ : ++tid_, a...);
    return map_[t->tid] = t;
  }
};

#endif
