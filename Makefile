CC=g++
CPPFLAGS=-std=c++11 -pthread

proxy: proxy.o clientInfo.o request.o response.o cache.o
	$(CC) $(CPPFLAGS) -o $@ proxy.o clientInfo.o request.o response.o cache.o

%.o: %.cpp
	$(CC) $(CPPFLAGS) -c -o $@ $<

.PHONY: clean
clean:
	rm -rf *.o *~ proxy

proxy.o: proxy.h clientInfo.h request.h exception.h cache.h
clientInfo.o: clientInfo.h exception.h
request.o: request.h exception.h
response.o: response.h exception.h
cache.o: response.h