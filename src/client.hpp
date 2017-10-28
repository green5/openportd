#include "main.h"

template<typename P> struct TLoc : TSocket::Parent
{
  P *parent;
  TSocket port;
  int64_t remote;
  size_t in,out;
  const int lport;
  TLoc(P *p,int64_t addr_,int port_):parent(p),port(this),remote(addr_),lport(port_)
  {
    in = out = 0;
    string h = format("localhost:%d",lport); /// bind local addr to 127.0.0.2
    port.connect(h);
    dlog("%p(%p): new loc fd=%s",this,remote,NAME(port.fd()));
  }
  string str() const
  {
    return format("%p(%p): %s io=%ld,%ld so=%p",this,(void*)remote,NAME(port.fd()),in,out,&port);
  }
  void write(const string &data)
  {
    if(lport==80) 
    {
      //check_http(data);
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
    out += write_data.size();
    parent->rpc.send(remote,'d',write_data);
    write_data.clear();
  }
  virtual void onread(int fd)
  {
    int n = TSocket::recv(fd, write_data);
    if(n==0)
    { 
      parent->rpc.send(remote,'e',"");
      parent->remove(this,__Line__);
      return;
    }
    if(lport==80)
    {
      http h;     
      int done = h.parse(write_data);
#if 0
      if(done==1)
      {
        flush();
        parent->rpc.send(remote,'e',"");
        parent->remove(this,__Line__);
        return;
      }
#endif
      if(done==0||done==1)
      {
        if(1==1 && h.head.find("Connection")==h.head.end())
        {
          h.head["Connection"] = "close";
        }
        else
        {
          for(auto &i:h.head) plog("%s: %s",i.first.c_str(),i.second.c_str());
          h.head["Connection"] = "close";
        }
        write_data = h.str();
      }
      flush();
    }
    else
    {
      flush();
    }
  }
};

struct Client : TSocket::Parent
{
  typedef TLoc<Client> Loc;
  typedef TChannel<Client> Channel;
  Config config;
  string buffer;
  Channel rpc;
  map<Loc*,int> loc_;
  vector<int> ports;
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
      rpc.send('b',Packet::b(port,config.get("pw")));
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
  Loc *find(Loc *loc)
  {
    auto i = loc_.find(loc);
    return i==loc_.end()?0:i->first;
  }
  void remove(Loc *loc,const Line &line)
  {
    Loc *p = find(loc);
    dlog("%s %s %p",line.C_STR(),loc->C_STR(),p);
    //rpc.send(p->remote,'f',"");
    loc_.erase(p);
    delete p;
  }
  void onreply(TSocket::Parent* p,Packet &packet)
  {
    plog("fd=%s unknown reply %s",NAME(rpc.fd()),packet.C_STR());
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
      	    rpc.send('L',format("bad port %d",c.port));
            break;
          }
        }
        Loc *loc = new Loc(this,c.addr,c.port);
        this->loc_[loc] = 1;
        rpc.reply(packet,Packet::c((int64_t)loc,-2)); ///?-2
        break;
      }
      case 'd':
      case 'e':
      case 'f':
      {
        Loc *loc = find((Loc*)packet.head.remote);
        if(loc==0) 
        {
          if(type!='e') plog("fd=%s unknown loc=%p %s",NAME(rpc.fd()),(void*)packet.head.remote,packet.C_STR());
        }
        else if(type=='e'||type=='f') remove(loc,__Line__);
        else if(type=='d') loc->write(packet.data);
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
