# openportd
Yet another tcp proxy

![ScreenShot](https://github.com/green5/openportd/blob/master/README.png?raw=true)

#### Usage

0. Get source
    git clone https://github.com/green5/openportd && cd openportd

1. Start on main server, port 40001 can be any admissible:

    make install;
    /usr/local/bin/openportd s.active=yes s.port=127.0.0.1:40001
   
2. Start on client:

    make install;
    /usr/local/bin/openportd c.active=yes c.port=Main-Server-Ip:40001 c.ports=22,80

3. Test, Port - port allocated by server, see server or client log:

    ssh -p Port Main-Server-Ip
  
