GCC_FLAGS = -g -std=gnu99 -Wall

all: client server

msg.c msg.h: msg.php
	php msg.php

msg.o: msg.c msg.h
	gcc -c $(GCC_FLAGS) msg.c

client: client.c msg.o
	gcc $(GCC_FLAGS) client.c msg.o -o client

server: server.c msg.o
	gcc $(GCC_FLAGS) server.c msg.o -o server

clean:
	rm -f *.o *.socket msg.c msg.h client server