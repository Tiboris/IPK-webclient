CXX=g++
CXXFLAGS=-O2 -g -Wall -Wextra -pedantic -std=c++11
LDFLAGS=-Wl,-rpath=/usr/local/lib/gcc49/
SOURCE=main.cpp
EXECUTABLE=-o webclient
RM=rm -f
all: 
	$(CXX) $(CXXFLAGS) $(EXECUTABLE) $(SOURCE) 
clean:
	$(RM) $(EXECUTABLE)