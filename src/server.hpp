#include "main.h"

template<typename P> struct TPub : TSocket::Parent
{
  tid_t tid;
  tid_t remote;
  P *parent;
  TSocket port;
  int lport;
  size_t in,out;
  TPub(tid_t tid_,P *p,int fd,int lport_):tid(tid_),remote(0),parent(p),port(this),lport(lport_),in(0),out(0)
  {
    port.connect(fd);
    dlog("%s",C_STR());
    Packet::c c(tid,lport);
    parent->rpc.call('c',c,this);
  }
  void onreply(Packet &reply)
  {
    auto c = reply.cast<Packet::c>();
    if(c.addr==0) parent->remove(this,__Line__);
    remote = c.addr;
    dlog("%p: remote=%d %s",this,remote,NAME(port.fd()));
    flush();
  }
  string str() const
  {
    return format("%p(#%d,#%d): %s io=%ld,%ld",this,tid,remote,NAME(port.fd()),in,out);
  }
  string write_data;
  void flush()
  {
    if(!remote) // f.e. ssh
    {
      dlog("null remote fd=%s data=%ld",NAME(port.fd()),write_data.size());
      return;
    }
    if(write_data.size())
    {
      out += write_data.size();
      parent->rpc.send(remote,'d',write_data);
      write_data.clear();
    }
  }
  void write(const string &data)
  {
    in += data.size();
    port.write(data);
  }
  virtual void onread(int fd)
  {
    int n = TSocket::recv(fd,write_data);
    if(write_data.size()==0)
    { 
      dlog("EOF fd=%s %s",NAME(fd),C_STR());
      parent->remove(this,__Line__);
      return;
    }
#if 1
    if(lport==80)
    {
      http h;
      int done = h.parse(write_data);
      if(done==0||done==1)
      {
        h.set("Connection","close");     
        write_data = h.data();
        flush();
      }
      else
      {
        plog("%s",h.str().c_str());
        plog("%s",dump(write_data).c_str());
      }
      return;
    }
#endif
    flush();
  }
};

template<typename P> struct TClient : TSocket::Parent
{
  typedef TPub<TClient> Pub;
  typedef TChannel<TClient> Channel;
  public:
  P *parent;
  Map<Pub> map_;
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
    string http = port==80 ? "http://" : port==443 ? "https://" : "";
    string log = format("listen on %s%s:%d for %d",http.c_str(),ext_ip.c_str(),aport,port);
    plog("%s",log.c_str());
    rpc.send(0,'L',log);
  }
  void finish(const Line &line,const char *cmd="none")
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
    int local = atoi(Socket(fd).local('p').c_str());
    if(port2.find(local)==port2.end()) 
    {
      plog("local=%d",local);
      return -1;
    }
    Pub *pub = map_.create(this,fd,port2[local]);
    /// и все?
    return 0;
  }
  void remove(Pub *pub,const Line &line)
  {
    dlog("%s %s",line.C_STR(),pub->C_STR());
    //rpc.send(pub->remote,'e',"");
    map_.erase(pub);
  }
  void onrpc(Packet &packet)
  {
    int type = packet.head.type;
    if(type=='b')
    {
      auto b = packet.cast<Packet::b>();
      //plog("version=%d port=%d",b.version,b.port);
      const string &pw = parent->config.get("pw");
      if(b.version!=VERSION)
      {
        rpc.send(0,'E',format("bad version, must %d",VERSION));
        return;
      }
      if(pw.size() && strcmp(pw.c_str(),b.pw)) 
      {
        rpc.send(0,'E',"bad pw");
        return;
      }
      auth = true;
    }    
    if(!auth)
    {
      finish(__Line__);
      return; 
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
        Pub *pub = map_.find(packet.head.to);
        if(pub==0)
        {
          if(type!='e') perr("fd=%s unknown pub %s",NAME(rpc.fd()),packet.C_STR());
        }
        else if(type=='e'||type=='f') remove(pub,__Line__);
        else if(type=='d') pub->write(packet.data);
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
  void finish(const Line &line,const char *cmd="break")
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

