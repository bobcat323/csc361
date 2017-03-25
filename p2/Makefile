all: \
compile

compile:
	gcc -o rdps rdps.c
	gcc -o rdpr rdpr.c

send:
	gcc -o rdps rdps.c
	./rdps 192.168.1.100 8081 10.10.1.100 8081 sent.dat

rec:
	gcc -o rdpr rdpr.c
	./rdpr 10.10.1.100 8081 received.dat

clean:
	rm rdps
	rm rdpr
	rm received.dat

#extra functions, to remove later
s:
	gcc -o client.out udpclient.c
	./client.out

r:
	gcc -o server.out udpserver.c
	./server.out

test:
	gcc -o test.out test.c
	./test.out
