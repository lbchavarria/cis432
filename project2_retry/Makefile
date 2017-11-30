CC=g++
#CFLAGS=-Wall -W -g -Werror 
# Uncomment this line for the graduate student version
CFLAGS= -g  -DGRAD=1

LOADLIBES= -lnsl

all: client server

client: client_v2.c raw.c
	$(CC) client_v2.c raw.c $(LOADLIBES) $(CFLAGS) -o client

server: server_v2.c 
	$(CC) server_v2.c $(LOADLIBES) $(CFLAGS) -o server

clean:
	rm -f client server *.o

