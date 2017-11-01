#include "main.h"

ConfigData Config::data;

namespace s
{
  #include "server.hpp"
}

namespace c
{
  #include "client.hpp"
}

string get_ext_ip()
{
  char line[1024];
  FILE *fin = popen("ip route get 8.8.8.8","r");
  if(fgets(line,sizeof(line),fin))
  {
    auto t = STD_H::split(line," ");
    if(t.size()>=7) return t[6];
  }
  return "0.0.0.0";
}

int SERVER = 0;
int DEBUG = 0;
int DAEMON = 0;
int SYSLOG = 0;

string ext_ip;
int sosync = 1; // for write, read always non-block
int noexit = 0;

int main(int ac,char *av[])
{
  ext_ip = get_ext_ip();
  //passert(sizeof(Header)==24,"Header=%ld",sizeof(Header));
  map_t arg;
  Config::main(ac,av,arg);
  for(auto &i:arg)
  {
    auto o = i.first;
    auto v = i.second;
    if(o=="help") 
    {
      plog("version %d",VERSION);
      pexit("usage: %s [--config=path] title.key=value ... [--b] [--debug]",av[0]);
    }
    else if(o=="b") DAEMON = 1;
    else if(o=="s") SERVER = 1;
    else if(o=="debug") DEBUG = atoi(v.c_str());
    else if(o=="syslog") SYSLOG = 1;
    else if(o=="exit") PEXIT;
    else if(o=="noexit") noexit = 1;
    else if(o=="sync") sosync = atoi(v.c_str());
    else pexit("bad option: --%s=%s",o.c_str(),v.c_str());
  }
  if(DAEMON)
  {
    SYSLOG = 1;
    noexit = 1;
    if(daemon(1,0)==-1) PEXIT;
  }
  for(int i=0;;i++)
  {
    if(SERVER)
    {
      s::Server().run();
    }
    else
    {
      c::Client().run();
    }
    plog("%s: %s %d",SERVER?"server":"client",noexit?"continue":"exit",i);
    if(!noexit) break;
    sleep(SERVER?5:15);
  } 
  return 0;
}

#include <syslog.h>
void mylog(std::string &t)
{
  if(SYSLOG)
  { 
    syslog(LOG_DAEMON|LOG_INFO,"%s",t.c_str());
  }
  else
  {
    printf("%s\n",t.c_str());
    fflush(stdout);
  }
}

void setport(struct sockaddr &a,int port)
{
  if(a.sa_family==AF_INET) ((sockaddr_in&)a).sin_port = htons(port);
  if(a.sa_family==AF_INET6) ((sockaddr_in6&)a).sin6_port = htons(port);
}

int getport(struct sockaddr &a)
{
  if(a.sa_family==AF_INET) return htons(((sockaddr_in&)a).sin_port);
  if(a.sa_family==AF_INET6) return htons(((sockaddr_in6&)a).sin6_port);
  return 0;
}
