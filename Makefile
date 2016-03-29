CXX=g++
CXXFLAGS=-O2 -g -Wall -Wextra -pedantic -std=c++11	
LDFLAGS=-Wl,-rpath=/usr/local/lib/gcc49/
APPNAME=webclient
RM=rm -f
all: $(APPNAME)

default: $(APPNAME)

$(APPNAME): $(APPNAME).cpp

clean:
	$(RM) $(APPNAME)
