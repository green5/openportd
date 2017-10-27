#include "main.h"

template<typename P> struct TPub : TSocket::Parent
{
  P *parent;
  TSocket port;
  int64_t remote;
  TPub(P *p,int fd,int dport):parent(p),port(this),remote(-1)
  {
    plog("%p: new pub fd=%s",this,NAME(fd));
    port.connect(fd);
    Packet::c c((int64_t)this,dport);
    parent->rpc.call('c',c,[this](Packet &reply){
      auto c = reply.cast<Packet::c>();
      if(c.addr==0) parent->remove(this);
      remote = c.addr;
    });
  }
  ~TPub()
  {
    parent->rpc.send(remote,'e',"");
    dlog("%p",this);
  }
  void write(const string &data)
  {
    port.write(data);
  }
  virtual void onread(int fd)
  {
    string data;
    int n = TSocket::recv(fd,data);
    if(n==0)
    { 
      dlog("EOF fd=%d",NAME(fd));
      parent->remove(fd);
      return;
    }
    parent->rpc.send(remote,'d',data);
  }
};

template<typename P> struct TClient : TSocket::Parent
{
  typedef TPub<TClient> Pub;
  typedef TChannel<TClient> Channel;
  public:
  P *parent;
  map<int,Pub*> pub;
  map<int,TSocket*> list; /// to pub?
  map<int,int> port2;
  Channel rpc;
  bool auth;
  public:
  TClient(P *p,int fd):parent(p),rpc(this),auth(false)
  {
    rpc.connect(fd);
  }
  string list_start(int port) 
  {
    auto i = pub.find(port);
    if(i!=pub.end())
    {
      delete i->second;
      pub.erase(i);        
    }
    TSocket *so = new TSocket(this);
    list[so->fd()] = so; /// or to pub
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
    port2[getport(addr)] = port;
    plog("listen on %d for %d",getport(addr),port);
    return name_(so->fd());
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
        auto h = list_start(b.port);
        if(b.port==80) rpc.send('L',format("start %d on %s [http://%s:%d]",b.port,h.c_str(),h.c_str(),b.port));
        else if(b.port==443) rpc.send('L',format("start %d on %s [https://%s:%d]",b.port,h.c_str(),h.c_str(),b.port));
	else rpc.send('L',format("start %d on %s",b.port,h.c_str()));
        break;
      }
      case 'x':
      {
        plog("finish by client fd=%s",NAME(rpc.fd()));
        finish(__Line__);
        break;
      }
      case 'd':
      case 'e':
      {
        auto i = std::find_if(pub.begin(),pub.end(),[&packet](const std::pair<int,Pub*> &a)
        {
          return packet.head.remote==(int64_t)a.second;
        });
        if(i==pub.end()) PLOG; 
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

