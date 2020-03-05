# make for Semaphores

CC=g++
CFLAGS = -Wall -lpthread 

client: client.o
	$(CC) $(CFLAGS) client.o -o client

server: server.o
	$(CC) $(CFLAGS) server.o -o server

clean:
	rm -f *.o *.bak client server

