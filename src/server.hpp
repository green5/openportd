#include "main.h"

template<typename P> struct TPub : TSocket::Parent
{
  P *parent;
  TSocket port;
  int64_t remote;
  size_t in,out;
  TPub(P *p,int fd,int dport):parent(p),port(this),remote(0)
  {
    in = out = 0;
    port.connect(fd);
    plog("%p: new pub fd=%s",this,NAME(port.fd()));
    Packet::c c((int64_t)this,dport);
    parent->rpc.call('c',c,[this](Packet &reply){
      auto c = reply.cast<Packet::c>();
      if(c.addr==0) parent->remove(this);
      remote = c.addr;
      dlog("%p(%p): remote %s",this,(void*)remote,NAME(port.fd()));
      flush();
    });
  }
  ~TPub()
  {
    //parent->rpc.send(remote,'e',"");
    plog("%p(%p): %s in=%ld out=%ld",this,(void*)remote,NAME(port.fd()),in,out);
  }
  string write_data;
  void flush()
  {
    if(!remote) // f.e. ssh
    {
      dlog("null remote fd=%s data=%ld",NAME(port.fd()),write_data.size());
      return;
    }
    out += write_data.size();
    parent->rpc.send(remote,'d',write_data);
    write_data.clear();
  }
  void write(const string &data)
  {
    in += data.size();
    port.write(data);
  }
  virtual void onread(int fd)
  {
    int n = TSocket::recv(fd,write_data);
    if(n==0)
    { 
      dlog("EOF fd=%s",NAME(fd));
      parent->remove(fd);
      return;
    }
    flush();
  }
};

template<typename P> struct TClient : TSocket::Parent
{
  typedef TPub<TClient> Pub;
  typedef TChannel<TClient> Channel;
  public:
  P *parent;
  map<int,Pub*> pub;
  map<int,TSocket*> listPort; /// to pub?
  map<int,int> port2;
  Channel rpc;
  bool auth;
  public:
  TClient(P *p,int fd):parent(p),rpc(this),auth(false)
  {
    rpc.connect(fd);
  }
  ~TClient()
  {
    dlog("%s",NAME(rpc.fd()));
    for(auto l:listPort) 
    {
      dlog("close list %d=%s",l.second->io.fd,NAME(l.second->io.fd));
      delete l.second;
    }
  }
  void list_start(int port) 
  {
    auto i = pub.find(port);
    if(i!=pub.end())
    {
      delete i->second;
      pub.erase(i);        
    }
    TSocket *so = new TSocket(this);
    struct sockaddr addr = unstr(AF_INET,parent->config.get("port"));
    string config = format("%s:%d",rpc.remote('h').c_str(),port);
    const string &pref = parent->config.get(config);
    setport(addr,pref.size()?atoi(pref.c_str()):0);
    so->listen(addr,5);
    if(pref.size()==0)
    {
      parent->config.set(config,so->local('p'));
      parent->config.data.write();
    }
    dlog("open list %d=%s",so->io.fd,NAME(so->io.fd));
    listPort[so->fd()] = so;
    int aport = getport(addr);
    port2[aport] = port;
    string http = port==80 ? "http://" : "";
    string log = format("listen on %s%s:%d for %d",http.c_str(),ext_ip.c_str(),aport,port);
    plog("%s",log.c_str());
    rpc.send('L',log);
  }
  void finish(const STD_H::Line &line,const char *cmd="none")
  {
    parent->remove(rpc.fd());
    parent->finish(line,cmd);
  }
  virtual void onerror(int fd)
  {
    plog("list error fd=%s",NAME(fd));
    finish(__Line__);
  }
  virtual int onlisten(int fd,sockaddr &addr)
  {
    if(pub.find(fd)!=pub.end()) remove(fd);
    int local = atoi(Socket(fd).local('p').c_str());
    dlog("local=%d",local);
    if(port2.find(local)==port2.end()) return -1;
    pub[fd] = new Pub(this,fd,port2[local]);
    return 0;
  }
  int remove(int fd)
  {
    auto i = pub.find(fd);
    if(i==pub.end())
    {
      plog("unregister pub fd=%s",NAME(fd));
    }
    else
    {
      delete i->second;
      pub.erase(i);
    }
  }
  void remove(Pub *pub_)
  {
    for(auto i:pub)
    {
      if(i.second==pub_) remove(i.first);
    }
  }
  void onrpc(int type,Packet &packet)
  {
    if(type=='b')
    {
      auto b = packet.cast<Packet::b>();
      //plog("version=%d port=%d",b.version,b.port);
      const string &pw = parent->config.get("pw");
      if(b.version!=VERSION)
      {
        rpc.send('E',format("bad version, must %d",VERSION));
        return;
      }
      if(pw.size() && strcmp(pw.c_str(),b.pw)) 
      {
        rpc.send('E',"bad pw");
        return;
      }
      auth = true;
    }    
    if(!auth)
    {
      return; /// ?call
    }
    switch(type)
    {
      case 'b':
      {
        auto b = packet.cast<Packet::b>();
        list_start(b.port);
        break;
      }
      case 'x':
      {
        plog("finish by client fd=%s",NAME(rpc.fd()));
        finish(__Line__);
        break;
      }
      case 'd':
      case 'f':
      case 'e':
      {
        auto i = std::find_if(pub.begin(),pub.end(),[&packet](const std::pair<int,Pub*> &a)
        {
          return packet.head.remote==(int64_t)a.second;
        });
        if(i==pub.end())
        {
          plog("fd=%s unknown pub=%p %s",NAME(rpc.fd()),(void*)packet.head.remote,packet.C_STR());
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

struct Server : TSocket::Parent
{
  typedef TClient<Server> Client;
  Config config;
  map<int,Client*> client;
  Server():config("s",{
    {"active","no"},
    {"port","0.0.0.0:40001"},
  })
  {
  }
  void run()
  {
    TSocket mainPort(this);
    mainPort.listen(config.get("port"));
    TSocket::loop("run",__Line__);
  }
  void finish(const STD_H::Line &line,const char *cmd="break")
  {
    TSocket::loop(cmd,line);
  }
  virtual void onerror(int fd)
  {
    plog(errno,"error fd=%s",NAME(fd));
    finish(__Line__);
  }
  virtual int onlisten(int fd,sockaddr &addr)
  {
    plog("new client %s",str(addr).c_str());
    if(client.find(fd)!=client.end()) remove(fd);
    client[fd] = new Client(this,fd);
    return 0;
  }
  void remove(int fd)
  {
    auto i = client.find(fd);
    if(i==client.end())
    {
      plog("unregister client fd=%s",NAME(fd));
    }
    else
    {
      delete i->second;
      client.erase(i);
    }
  }
};

