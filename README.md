# openportd
Yet another tcp proxy

![ScreenShot](https://github.com/green5/openportd/blob/master/README.png?raw=true)

Usage:
  1. Start on main server, port 60001 can be any admissible:
    make install && /usr/local/bin/openportd s.active=yes s.port=127.0.0.1:60001
     
  2. Start on client:
    make install && /usr/local/bin/openportd c.active=yes c.port=Main-Server-Ip:60001

  3. Test, Port - port allocated by server, see server or client log:
    ssh -p Port Main-Server-Ip
  