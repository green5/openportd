CXXFLAGS += -std=c++14 -pthread -I./include

CXXFLAGS += -DUSE_EV
LIB += -lev

#CXXFLAGS += -DUSE_LEV
#LIB += -levent

SRC=$(shell echo src/*)

all: apt-install openportd

openportd: $(SRC) Makefile
	g++ -g -O0 ${CXXFLAGS} -o openportd src/main.cpp ${LIB}

f:
	g++ -g -O0 ${CXXFLAGS} -o openportd src/main.cpp ${LIB}

s: openportd
	./openportd s.active=yes c.active=no

ss: openportd
	gdb.a ./openportd s.active=yes c.active=no

c: openportd
	set -m; ./openportd s.active=no c.active=yes

c2: openportd
	set -m; ./openportd --config=/tmp/o.conf s.active=no c.active=yes

cc: openportd
	gdb.a ./openportd s.active=no c.active=yes

ed:
	nano ~/.openportd.conf

a=/usr/include/ev++.h

apt-install:
	@[ -r $(a) ] || echo "Install $(a) ..."
	@[ -r $(a) ] || sudo apt-get install libev-dev

install: openportd
	sudo -u root sh -c "mkdir -p /usr/local/bin && cp -va openportd /usr/local/bin"

clean:
	rm -f openportd
