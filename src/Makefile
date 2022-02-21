CC=g++
CPPFLAGS=-std=c++11 -pthread
SRCS=$(wildcard *.cpp)
OBJS=$(patsubst %.cpp, %.o, $(SRCS))

proxy: $(OBJS)
	$(CC) $(CPPFLAGS) -o $@ $(OBJS)

%.o: %.cpp
	$(CC) $(CPPFLAGS) -c -o $@ $<

.PHONY: clean
clean:
	rm -rf *.o *~ var proxy

proxy.o: proxy.h clientInfo.h request.h exception.h cache.h
clientInfo.o: clientInfo.h exception.h
request.o: request.h exception.h
response.o: response.h exception.h customTime.h
cache.o: request.h response.h