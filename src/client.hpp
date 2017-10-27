#include "main.h"

template<typename P> struct TLoc : TSocket::Parent
{
  P *parent;
  TSocket port;
  int64_t remote;
  TLoc(P *p,int64_t addr_,int port_):parent(p),port(this),remote(addr_)
  {
    string h = format("localhost:%d",port_);
    dlog("%p(%p): loc connect to %s",this,remote,h.c_str());
    port.connect(h);
  }
  ~TLoc()
  {
    dlog("%p",this);
  }
  void write(const string &data)
  {
    port.write(data);
  }
  virtual void onread(int fd)
  {
    string data;
    int n = TSocket::recv(fd, data);
    if(n==0)
    { 
      parent->rpc.send(remote,'e',"");
      parent->remove(fd);
      return;
    }
    parent->rpc.send(remote,'d',data);
  }
};

struct Client : TSocket::Parent
{
  typedef TLoc<Client> Loc;
  typedef TChannel<Client> Channel;
  Config config;
  string buffer;
  Channel rpc;
  map<int,Loc*> loc;
  vector<int> ports;
  Client():config("c",{
    {"active","no"},
    {"port","127.0.0.1:40001"},
    {"ports","22,80,23"},
  }),rpc(this)      
  {
    for(auto p:STD_H::split(config.get("ports").c_str(),",")) ports.push_back(atoi(p.c_str()));
  }
  void run()
  {
    rpc.connect(config.get("port"));
    for(auto port:ports)
    {
      rpc.send('b',Packet::b(port,config.get("pw")));
    }
    TSocket::loop("run",__Line__);
  }
  virtual void onerror(int fd,const STD_H::Line &line)
  {
    plog(errno,"%s fd=%s",line.str().c_str(),NAME(fd));
    TSocket::loop("break",__Line__);
  }
  void start(int xid,int port)
  {
  }
  void finish(const STD_H::Line &line,const char *cmd="break")
  {
    TSocket::loop(cmd,line);
  }
  int remove(int fd)
  {
    auto i = loc.find(fd);
    if(i==loc.end())
    {
      plog("unregister loc fd=%s",NAME(fd));
    }
    else
    {
      delete i->second;
      loc.erase(i);
    }
  }
  void remove(Loc *loc_)
  {
    for(auto i:loc)
    {
      if(i.second==loc_) remove(i.first);
    }
  }
  void onrpc(int type,Packet &packet)
  {
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
	    rpc.send('L',format("bad port %d",c.port));
            break;
          }
        }
        Loc *loc = new Loc(this,c.addr,c.port);
        this->loc[loc->port.fd()] = loc;
        rpc.reply(packet,Packet::c((int64_t)loc,-2)); ///?-2
        break;
      }
      case 'd':
      case 'e':
      {
        auto i = std::find_if(loc.begin(),loc.end(),[&packet](const std::pair<int,Loc*> &a)
        {
          return packet.head.remote==(int64_t)a.second;
        });
        if(i==loc.end()) 
        {
          if(type!='e') plog("fd=%s unknown remote %s",NAME(rpc.fd()),packet.C_STR());
        }
        else if(type=='e') remove(i->second);
        else if(type=='d') i->second->write(packet.data);
        break;
      }
      default:
      {         
        plog("fd=%s unknown packet %s",NAME(rpc.fd()),packet.C_STR());
        finish(__Line__);
        break;
      }
    }
  }
};
