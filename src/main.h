#ifndef MAIN_H_
#define MAIN_H_

#define VERSION 1

#include <string>
void mylog(std::string&);
#define MYLOG mylog

#include "std.h"
#include "std/str.h"

string name_(int fd,int which=3);
void setport(struct sockaddr&,int port);
int getport(struct sockaddr &a);

#define NAME(fd) name_(fd).c_str()
#define C_STR str().c_str
#define PCHAR(c) ((c<=' '||c>=127)?'.':c)
extern int DEBUG;
static const string null;

typedef std::map<string,string> map_t;
using STD_H::format;
using STD_H::str;
using STD_H::unstr;
using STD_H::dump;

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
};

typedef struct EvSocket TSocket;
struct EvSocket
{
  struct Parent
  {
    virtual void onerror(int fd,const STD_H::Line &line)
    {
      plog(errno,"%s fd=%s",line.C_STR(),NAME(fd));
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
  }
  virtual ~EvSocket()
  {
    close(io.fd);
  }
  int fd()
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
  static int socket(int s, struct sockaddr *a=0)
  {
    if(s==-1) s = ::socket(a->sa_family, SOCK_STREAM, 0);
    if(s==-1) PEXIT;
    int opt = 1;
    if(setsockopt(s,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt))==-1) PEXIT;
    if(fcntl(s,F_SETFL,fcntl(s,F_GETFL,0)|O_NONBLOCK)==1) PEXIT; 
    return s;
  }
  EvSocket& listen(sockaddr &addr,int backlog)
  {
    int s = socket(-1,&addr);
    if(bind(s,&addr,sizeof(addr))==-1) PEXIT;
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
    s = socket(s);
    io.set<EvSocket,&EvSocket::on_connect_socket>(this);
    io.start(s,ev::READ);
    return *this;
  }
  EvSocket& connect(const string &port)
  {
    struct sockaddr addr = unstr(AF_INET,port);
    int s = socket(-1,&addr);
    if(::connect(s,&addr,sizeof(addr))==-1) 
    {
      if(!(errno==EINPROGRESS)) PEXIT;
    }
    sleep(3);
    plog("%s: connect to fd=%s",str(addr).c_str(),NAME(s));
    return connect(s);
  }
  static ev::default_loop loop_;
  static void loop(const string &cmd,const STD_H::Line &line)
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
    if(n!=a.size()) parent->onerror(io.fd,__Line__);
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

struct Header
{
  unsigned size;
  unsigned type;
  unsigned xid;
  int64_t remote;
  string str() const
  {
    return format("size=%d (%c) xid=%d remote=%p",size,type,xid,(void*)remote);
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
    if(buf.size()<packet.head.size) return 0;
    packet.data = buf.substr(sizeof(head),packet.head.size-sizeof(head));
    buf = buf.substr(packet.head.size); // fixs, optimize later
    return 1;
  }
  string str() const
  {
    return format("%s data=%ld+%ld",head.C_STR(),sizeof(head),data.size());
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
    int64_t addr;
    int port;
    c(int64_t addr_,int port_):addr(addr_),port(port_){}
  };
};

template<typename P> struct TChannel : TSocket, TSocket::Parent
{
  typedef std::function<void(Packet&)> ret_t;
  P *parent;
  int xid_;
  std::map<int,ret_t> rpc;
  TChannel(P*p):TSocket(this),parent(p),xid_(0){}
  void send(int type,const void *data=0,int size=0,int xid=0,int64_t remote=0)
  {
    Header head;
    head.size = sizeof(head) + size;
    head.type = type;
    head.xid = xid;
    head.remote = remote;  
    if(DEBUG==1) plog("[%s] size=%d",head.C_STR(),size);
    if(DEBUG==2) plog("[%s] size=%d %s",head.C_STR(),size,dump(data,size).c_str());
    struct iovec io[2];
    io[0].iov_base = &head;
    io[0].iov_len = sizeof(head);
    io[1].iov_base = (void*)data;
    io[1].iov_len = size;
    ssize_t n = writev(fd(),io,data?2:1);
    if(n!=head.size) plog("n=%ld h=%ld+%ld",n,sizeof(head),size);
  }
  void send(int type,const string &data)
  {
    return send(type,data.c_str(),data.size());
  }
  void send(int64_t remote,int type,const string &data)
  {
    return send(type,data.c_str(),data.size(),0,remote);
  }
  template<typename T> void send(int type,const T& t)
  {
    return send(type,&t,sizeof(t));
  }
  template<typename T> void call(int type,const T& t,const ret_t &ret)
  {
    int xid = ++xid_ ? xid_ : ++xid_;
    rpc[xid] = ret;
    return send(type,&t,sizeof(t),xid);
  }
  template<typename T> void reply(Packet &o,const T& t)
  {
    return send('R',&t,sizeof(t),o.head.xid);
  }
  std::list<Packet*> write_queue;
  std::string read_buffer;
  virtual void onwrite(int fd)
  {
    io.set(write_queue.empty()?ev::READ:ev::READ|ev::WRITE);
  }
  void read(Packet &packet)
  {
    if(DEBUG==1) plog("[%s]",packet.C_STR());
    if(DEBUG==2) plog("[%s]%s",packet.C_STR(),dump(packet.data).c_str());
    switch(packet.head.type)
    {
      case 'L':
      {
        plog("L:%s",packet.data.c_str());
        break;
      }
      case 'E':
      {
        plog("E:%s",packet.data.c_str());
        parent->finish(__Line__,"exit"); /// server?
        break;
      }
      case 'R':
      {
        auto i = rpc.find(packet.head.xid);
        if(i!=rpc.end())
        {
          i->second(packet);
          rpc.erase(i);
        }
        else
          plog("fd=%s bad reply=%s",NAME(TSocket::fd()),packet.C_STR());
        break;
      }
      default:
      {
        parent->onrpc(packet.head.type,packet);
        break;
      }
    }
  }
  virtual void onread(int fd)
  {
    size_t n = TSocket::recv(fd, read_buffer);
    if(n==0)
    {
      plog(errno,"fd=%s",NAME(fd));
      //if(errno!=EINPROGRESS)
      parent->finish(__Line__);
      return;
    }
    Packet packet;
    while(Packet::unpack(read_buffer,packet)) read(packet);
  }
  virtual void onerror(int fd,const STD_H::Line &line)
  {
    plog("%s: rpc error fd=%s",line.C_STR(),NAME(fd));
    parent->finish(__Line__);
  }
};

#endif
                       
