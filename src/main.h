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
    //plog("close %d=%s",io.fd,NAME(io.fd));
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
  static int socket_(struct sockaddr *a)
  {
    int s = ::socket(a->sa_family, SOCK_STREAM, 0);
    if(s==-1) PEXIT;
    return s;
  }
  virtual void setoption(int fd)
  {
    //if(fcntl(fd,F_SETFL,fcntl(fd,F_GETFL,0)|O_NONBLOCK)==-1) PERR; 
  }
  EvSocket& listen(sockaddr &addr,int backlog)
  {
    int s = socket_(&addr);
    int opt = 1;
    setoption(s);
    if(setsockopt(s,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt))==-1) PERR;
    if(bind(s,&addr,sizeof(addr))==-1)
    {
      pexit(errno,"%s",str(addr).c_str());
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
  EvSocket& connect(const string &port)
  {
    struct sockaddr addr = unstr(AF_INET,port);
    int s = socket_(&addr); 
    if(::connect(s,&addr,sizeof(addr))==-1) 
    {
      //if(!(errno==EINPROGRESS)) 
      PEXIT;
    }
    setoption(s);    
    io.set<EvSocket,&EvSocket::on_connect_socket>(this);
    io.start(s,ev::READ);
    return *this;
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
  TChannel(P*p):TSocket(this),parent(p),xid_(0)
  {
  }
  virtual void setoption(int fd)
  {
    //if(fcntl(fd,F_SETFL,fcntl(fd,F_GETFL,0)|O_NONBLOCK)==-1) PERR; 
    int r1 = Socket(fd).get(SO_RCVBUF);
    int s1 = Socket(fd).get(SO_SNDBUF);
    Socket(fd).set(SO_SNDBUF,1000000);
    Socket(fd).set(SO_RCVBUF,1000000);
    int r2 = Socket(fd).get(SO_RCVBUF);
    int s2 = Socket(fd).get(SO_SNDBUF);
    plog("rs_buf(%s) %d,%d -> %d,%d",NAME(fd),r1,s1,r2,s2);
  }
  void send(int type,const void *data=0,int size=0,int xid=0,int64_t remote=0)
  {
    Header head;
    head.size = sizeof(head) + size;
    head.type = type;
    head.xid = xid;
    head.remote = remote;  
    if(DEBUG==2) plog("[%s] size=%d",head.C_STR(),size);
    if(DEBUG==3) plog("[%s] size=%d %s",head.C_STR(),size,dump(data,size).c_str());
    struct iovec io[2];
    io[0].iov_base = &head;
    io[0].iov_len = sizeof(head);
    io[1].iov_base = (void*)data;
    io[1].iov_len = data ? size : 0;
    ssize_t n = writev(fd(),io,data?2:1);
    if(n!=head.size) plog(errno,"fd=%s n=%ld v=%ld",NAME(fd()),n,sizeof(head)+size);
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
    if(DEBUG==2) plog("[%s]",packet.C_STR());
    if(DEBUG==3) plog("[%s]%s",packet.C_STR(),dump(packet.data).c_str());
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
      //if(!(errno==ENOTCONN))
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
                       
