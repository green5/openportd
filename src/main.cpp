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

int DEBUG = 0;
int DAEMON = 0;
int SYSLOG = 0;

string ext_ip;

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
    else if(o=="b") DAEMON = true;
    else if(o=="debug") DEBUG = atoi(v.c_str());
    else if(o=="syslog") SYSLOG = true;
    else if(o=="exit") PEXIT;
    else pexit("bad option: --%s=%s",o.c_str(),v.c_str());
  }
  if(DAEMON)
  {
    SYSLOG = true;
    if(daemon(1,0)==-1) PEXIT;
  }
  if(Config::data.get("s","active")=="yes")
  {
    s::Server().run();
  }
  if(Config::data.get("c","active")=="yes")
  {
    c::Client().run();
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
