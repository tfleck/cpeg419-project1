CC= /usr/bin/gcc

all:	tcpclient tcpserver

tcpclient: tcpclient.c;
	${CC} tcpclient.c -o tcpclient

tcpserver: tcpserver.c;
	${CC} tcpserver.c -o tcpserver

clean:
	rm tcpclient tcpserver
