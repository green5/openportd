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

s: openportd kill
	$${x} ./openportd --s s.port=0.0.0.0:40001 --debug=$${z}

b: kill openportd
	./openportd --s --b s.port=0.0.0.0:40001

ss: openportd kill
	$${x} ./openportd --s s.port=0.0.0.0:40001 --debug=$${z}

c: openportd
	$${x} ./openportd --debug=$${z} c.ports=22,80,443 c.port=u7:40001

cc: openportd
	$${x} ./openportd --debug=$${z} c.ports=22,80,443 c.port=mp:40001 --noexit

bb: openportd
	./openportd --b c.ports=22,80,443 c.port=mp:40001

ed:
	nano ~/.openportd.conf

apt-install:
	@[ -r $(TEST) ] || echo "Install $(PKG) ..."
	@[ -r $(TEST) ] || sudo apt-get install $(PKG)

install: all
	sudo -u root sh -c "mkdir -p /usr/local/bin && make kill && cp -va openportd /usr/local/bin"

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

kill:
	(pkill -e openportd; exit 0)
