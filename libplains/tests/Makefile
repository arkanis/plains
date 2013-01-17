GCC_FLAGS = -std=gnu99 -g -Wall
PHP_OPTS = -d short_open_tag=on

msg_test: msg_test.c
	cd ..; make msg.o
	gcc $(GCC_FLAGS) msg_test.c ../msg.o -o msg_test

clean:
	rm -f *.o msg_test