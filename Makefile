
all: letmein-tcp letmein-udp server-tcp server-udp

letmein-tcp:
	gcc -o letmein-tcp TCP/Client/*.c 

server-tcp: 
	gcc -o server-tcp TCP/Server/*.c 

letmein-udp: 
	gcc -o letmein-udp UDP/Client/*.c

server-udp: 
	gcc -o server-udp UDP/Server/*.c

clean:
	/bin/rm letmein-tcp letmein-udp server-tcp server-udp

