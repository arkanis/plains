GCC_FLAGS = -std=gnu99 -g -Wall
PHP_OPTS = -d short_open_tag=on

msg_test: msg_test.c
	cd ..; make msg.o
	gcc $(GCC_FLAGS) msg_test.c ../msg.o -o msg_test

client_test: client_test.c
	cd ..; make libplains.o
	gcc $(GCC_FLAGS) client_test.c ../libplains.o ../msg.o ../net.o -o client_test

server_test: server_test.c
	cd ..; make libplains.o
	gcc $(GCC_FLAGS) server_test.c ../msg.o ../net.o -o server_test

clean:
	rm -f *.o *_test core server.socket