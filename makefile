
full: server client

server: server.o shared.o
	gcc -o server server.o shared.o

client: client.o shared.o
	gcc -o client client.o shared.o

server.o: server.c
	gcc -c server.c

client.o: client.c
	gcc -c client.c

shared.o: shared.c
	gcc -c shared.c

clean:
	rm *.o server client