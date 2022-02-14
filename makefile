all:
	g++ -Wall -c common.cpp 
	g++ -Wall emitter.cpp common.o -o emitter
	g++ -Wall exhibitor.cpp common.o -o exhibitor
	g++ -Wall server.cpp common.o -o server 
clean:
