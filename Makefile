CXXFLAGS += -std=c++11 -pthread -I./include

CXXFLAGS += -DUSE_EV
LIB += -lev

#CXXFLAGS += -DUSE_LEV
#LIB += -levent

SRC=$(shell echo src/*)

all: apt-install openportd

openportd: $(SRC)
	g++ -g -O0 ${CXXFLAGS} -o openportd src/main.cpp ${LIB}

f:
	g++ -g -O0 ${CXXFLAGS} -o openportd src/main.cpp ${LIB}

s: openportd
	./openportd s.active=yes c.active=no --debug=0

ss: openportd
	gdb.a ./openportd s.active=yes c.active=no --debug=1

c: openportd
	./openportd s.active=no c.active=yes c.port=mp:40001 --debug=0

cc: openportd
	gdb.a ./openportd s.active=no c.active=yes --debug=1

ed:
	nano ~/.openportd.conf

a=/usr/include/ev++.h

apt-install:
	@[ -r $(a) ] || echo "Install $(a) ..."
	@[ -r $(a) ] || sudo apt-get install libev-dev

install: all
	sudo -u root sh -c "mkdir -p /usr/local/bin && cp -va openportd /usr/local/bin"

clean:
	rm -f openportd

commit:
	git add .
	git commit -m "make commit"
	git push -u origin master

pull:
	(git status | grep modified: && exit 1 || exit 0)
	git checkout .
	git pull
	make
