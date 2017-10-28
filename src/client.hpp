#include "main.h"

template<typename P> struct TLoc : TSocket::Parent
{
  tid_t tid;
  tid_t remote;
  P *parent;
  TSocket port;
  const int lport;
  size_t in,out;
  TLoc(tid_t tid_,P *p,tid_t fr,int port_):tid(tid_),remote(fr),parent(p),port(this),lport(port_),in(0),out(0)
  {
    dlog("%s",C_STR());
    string h = format("localhost:%d",lport); /// bind local addr to 127.0.0.2
    port.connect(h);
  }
  string str() const
  {
    return format("%p(#%d,#%d): %s io=%ld,%ld",this,tid,remote,NAME(port.fd()),in,out);
  }
  void onreply(Packet &reply)
  {
    plog("%s",reply.C_STR());
  }
  void write(const string &data)
  {
    if(data.size()==0)
    {
      PLOG;
      return;
    }
    in += data.size();
    port.write(data);
  }
  string write_data;
  void flush()
  {
    if(!remote)
    {
      pexit("null REMOTE fd=%s data=%ld",NAME(port.fd()),write_data.size());
      return;
    }
    if(write_data.size())
    {
      out += write_data.size();
      parent->rpc.send(remote,'d',write_data);
      write_data.clear();
    }
  }
  virtual void onread(int fd)
  {
    int n = TSocket::recv(fd, write_data);
    if(n==0)
    { 
      if(remote) parent->rpc.send(remote,'e',"");
      remote = 0; // forget remote
      parent->remove(this,__Line__);
      return;
    }
    flush();
  }
};

struct Client : TSocket::Parent
{
  typedef TLoc<Client> Loc;
  typedef TChannel<Client> Channel;
  Config config;
  Channel rpc;
  Map<Loc> map_;
  vector<int> ports;
  string buffer;
  Client():config("c",{
    {"active","no"},
    {"port","127.0.0.1:40001"},
    {"ports","22,80"},
  }),rpc(this)      
  {
    for(auto p:STD_H::split(config.get("ports").c_str(),",")) ports.push_back(atoi(p.c_str()));
  }
  void run()
  {
    rpc.connect(config.get("port"));
    for(auto port:ports)
    {
      rpc.send(0,'b',Packet::b(port,config.get("pw")));
    }
    TSocket::loop("run",__Line__);
  }
  virtual void onerror(int fd,const Line &line)
  {
    plog(errno,"%s fd=%s",line.str().c_str(),NAME(fd));
    TSocket::loop("break",__Line__);
  }
  void start(int xid,int port)
  {
  }
  void finish(const Line &line,const char *cmd="break")
  {
    TSocket::loop(cmd,line);
  }
  void remove(Loc *loc,const Line &line)
  {
    dlog("%s %s",line.C_STR(),loc->C_STR());
    //rpc.send(loc->remote,'f',"");
    map_.erase(loc);
  }
  void onrpc(Packet &packet)
  {
    int type = packet.head.type;
    switch(type)
    {
      case 'c':
      {
        auto c = packet.cast<Packet::c>();
        if(1==1)
        {
      	  auto a = std::find(ports.begin(),ports.end(),c.port);
      	  if(a==ports.end())
      	  {
      	    rpc.reply(packet,Packet::c(0,-1));
      	    rpc.send(0,'L',format("bad port %d",c.port));
            break;
          }
        }
        Loc *loc = map_.create(this,c.addr,c.port);
        rpc.reply(packet,Packet::c(loc->tid)); 
        break;
      }
      case 'd':
      case 'e':
      case 'f':
      {
        Loc *loc = map_.find(packet.head.to);
        if(loc==0) 
        {
          if(type!='e') perr("fd=%s unknown loc %s",NAME(rpc.fd()),packet.C_STR());
        }
        else if(type=='e'||type=='f') remove(loc,__Line__);
        else if(type=='d') loc->write(packet.data);
        break;
      }
      default:
      {         
        perr("fd=%s unknown packet %s",NAME(rpc.fd()),packet.C_STR());
        finish(__Line__);
        break;
      }
    }
  }
};
