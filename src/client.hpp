#include "main.h"
#include <bsd/string.h>

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
    xlog("%p(%p): new loc fd=%s",this,remote,NAME(port.fd()));
  }
  ~TLoc()
  {
    //parent->rpc.send(remote,'f',"");
    xlog("%p(%p): %s in=%ld out=%ld",this,(void*)remote,NAME(port.fd()),in,out);
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
      parent->remove(fd);
      return;
    }
    bool done = false;
    if(lport==80) /// 443
    {
      done = check_http(write_data);
      if(!done) { dlog("http continue"); return; }
    }
    flush();
    if(done)
    {
      parent->rpc.send(remote,'e',"");
      parent->remove(fd);
      return;
    }
  }
  bool check_http(const string &data)
  {
    if(data.size()==0)
    {
      plog("NULL");
      return false;
    }
    map_t h;
    char *t, *s, *e;
    s = (char*)data.c_str();
    e = s + data.size();
    for(int i=0;s<e;i++,s=t+2)
    {
      t = strnstr(s,"\r\n",e-s); //t = strstr(s,"\r\n");
      if(t==0) { PLOG; break; }
      if(t==s) { t += 2; break; }
      if(i==0)
      {
        h["method"] = string(s,0,t-s);
      }
      else
      {
        char *c = strnstr(s,": ",t-s);
        if(c==0) PLOG;
        else h[string(s,c-s)] = string(c+2,t-c-2);
        //h.push_back(string(s,0,t-s));
      }
    }
    s = (char*)data.c_str();
    size_t hlen = t - s;
    size_t tlen = s + data.size() - t;
    // tail = Content-Length (if there is)
    // Content-Encoding: gzip (size[4] + "\n" + h[8] + ...)
    // data[Content-Length]
    if(tlen==0) return true; // only headers
    if(h["Content-Encoding"]=="gzip")
    {
      size_t nz = strtol(string(t,4).c_str(),NULL,16) + 4 + 1 + 8;
      //if(nz!=tlen) plog("nz=%d tail=%ld %s",nz,tlen,dump(data).c_str());
      return nz <= tlen;
    }
    if(h["Content-Length"].size())
    {
      size_t nz = strtol(h["Content-Length"].c_str(),NULL,10);
      //if(nz!=tlen) plog("nz=%d tail=%ld %s",nz,tlen,dump(data).c_str());
      return nz <= tlen;
    }
    plog("data=%ld tail=%d",data.size(),tlen);
    for(auto &i:h) plog("%s: %s",i.first.c_str(),i.second.c_str());
    return false;
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
      case 'f':
      {
        auto i = std::find_if(loc.begin(),loc.end(),[&packet](const std::pair<int,Loc*> &a)
        {
          return packet.head.remote==(int64_t)a.second;
        });
        if(i==loc.end()) 
        {
          if(type!='e') plog("fd=%s unknown loc=%p %s",NAME(rpc.fd()),(void*)packet.head.remote,packet.C_STR());
        }
        else if(type=='e'||type=='f') remove(i->second);
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
