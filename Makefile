CXXFLAGS += -std=c++11 -pthread -I./include

CXXFLAGS += -DUSE_EV
LIB += -lev -lbsd
PKG = libbsd-dev libev-dev
TEST = /usr/include/ev++.h

#CXXFLAGS += -DUSE_LEV
#LIB += -levent

SRC=$(shell echo src/*)

all: apt-install openportd

openportd: $(SRC)
	g++ -g -O0 ${CXXFLAGS} -o openportd src/main.cpp ${LIB}

f:
	g++ -g -O0 ${CXXFLAGS} -o openportd src/main.cpp ${LIB}

s: openportd
	$${x} ./openportd s.active=yes c.active=no --debug=$${z}

ss: openportd
	$${x} ./openportd s.active=yes c.active=no --debug=$${z}

c: openportd
	$${x} ./openportd s.active=no c.active=yes --debug=$${z} c.port=mp:40001

cc: openportd
	$${x} ./openportd s.active=no c.active=yes --debug=$${z} c.port=localhost:40001

ed:
	nano ~/.openportd.conf

apt-install:
	@[ -r $(TEST) ] || echo "Install $(PKG)"
	@[ -r $(TEST) ] || sudo apt-get install $(PKG)

install: all
	sudo -u root sh -c "mkdir -p /usr/local/bin && cp -va openportd /usr/local/bin"

clean:
	rm -f openportd

commit:
	make
	git add .
	git commit -m "make commit"
	git push -u origin master

pull:
	(git status | grep modified: && exit 1 || exit 0)
	git checkout .
	git pull
	make
