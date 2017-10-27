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

int DEBUG = 0;
int DAEMON = 0;
int SYSLOG = 0;

int main(int ac,char *av[])
{
  map_t arg;
  Config::main(ac,av,arg);
  for(auto &i:arg)
  {
    auto o = i.first;
    auto v = i.second;
    if(o=="help") 
    {
      plog("version %d",VERSION);
      pexit("usage: %s [--config=path] title.key=value ... [-b] [--debug]",av[0]);
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
  }
}

string name_(int fd,int which)
{
  string ret;
  struct sockaddr name;
  socklen_t len;
  if(which&1)
  {
    len = sizeof(name);
    ret += getsockname(fd,&name,&len) == 0 ? str(name) : "ERR";
  }
  if(which&2)
  {
    len = sizeof(name);
    ret += ",";
    ret += getpeername(fd,&name,&len) == 0 ? str(name) : "ERR";
  }
  return "[" + ret + "]";
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
