struct ConfigData
{
  string path;
  bool needSave;
  map<string,map_t> data;
  ConfigData(const map<string,map_t> &config_={}):needSave(false)
  {
  }
  void read()
  {
    if(!path.size())
    {
      path = format("%s/.openportd.conf",getenv("HOME"));
    }
    FILE *fin = 0;
    if(!fin) fin = fopen(path.c_str(),"r+");
    if(!fin) 
    {
      plog("create %s",path.c_str());
      fin = fopen(path.c_str(),"w+");
    }
    if(!fin) pexit("can't create %s",path.c_str());
    char t[1000];
    string title;
    while(fgets(t,sizeof(t),fin))
    {
      if(t[0]=='[') 
      {
        title = strtok(t,"[]\r\n");
        continue;
      }
      strtok(t,"\r\n");
      if(title.size())
      {
        char *v = strchr(t,'=');
        if(v) *v++ = 0; else v = (char*)"yes";
        set(title,t,v);
      }
    }
    fclose(fin);
    needSave = false;
  }
  void write()
  {
    if(!needSave) return;
    FILE *fout = fopen(path.c_str(),"w+");
    for(auto &i:data)
    {
      fprintf(fout,"[%s]\n",i.first.c_str());
      for(auto &j:i.second)
      {
        fprintf(fout,"%s=%s\n",j.first.c_str(),j.second.c_str());
      }      
    }
    fclose(fout);
    needSave = false;
  }
  void set(const string &t,const string &a,const string &b)
  {
    auto &config = data[t];
    if(config.find(a)==config.end()||config[a]!=b)
    {
      needSave = true;
      config[a] = b;
    }
  }
  const string &get(const string &t,const string &a)
  {
    auto &config = data[t];
    auto i = config.find(a);
    if(i==config.end())
    {
      //plog("null key %s.%s",t.c_str(),a.c_str());
      return null;
    }
    return i->second;
  }
};

struct Config
{
  static ConfigData data;
  string title;
  map_t *config;
  Config(const char *title_,const map<string,string> &config_={}):title(title_)
  {
    config = (map_t*)&data.data[title];
    for(auto &i:config_) 
    {
      if(config->find(i.first)==config->end()) set(i.first,i.second);
    }
    data.write();
  }  
  void set(const string& a,const string& b)
  {
    data.set(title,a,b);
    data.write();
  }
  const string &get(const string& a)
  {
    return data.get(title,a);
  }
  static void main(int ac,char *av[],map_t &map)
  {
    for(int i=1;i<ac;i++)
    {
      string t(av[i]);
      char *a = (char*)t.c_str();
      if(a[0]=='-'&&a[1]=='-')
      {
        string key(a+2);
        char *val = strchr((char*)key.c_str(),'=');
        if(val) *val++ = 0, key = key.c_str(); else val = (char*)"yes";
        if(key=="help-config") 
        {
        }
        else if(!strcmp(key.c_str(),"config")) 
        {
          plog("config: %s",val);
          data.path = val;
        }
        else 
        {
          map[key] = val;
        }
      }
    }
    data.set("s","active","no");
    data.set("c","active","no");
    data.read();
    for(int i=1;i<ac;i++)
    {
      string t(av[i]);
      char *a = (char*)t.c_str();
      if(!(a[0]=='-'&&a[1]=='-'))
      {
        char *v = strchr(a,'=');
        if(v) *v++ = 0; else v = (char*)"yes";
        char *k = strchr(a,'.');
        if(k==0) pexit("no option title: %s",a);
        *k++ = 0;
        data.set(a,k,v);
      }
    }
    data.write();
  }
};
