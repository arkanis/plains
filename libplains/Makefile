GCC_FLAGS = -std=gnu99 -g -Wall
PHP_OPTS = -d short_open_tag=on

msg.h: msg.h.php protocol.php
	php $(PHP_OPTS) msg.h.php > msg.h

msg.c: msg.c.php msg.h
	php $(PHP_OPTS) msg.c.php > msg.c

msg.o: msg.c msg.h
	gcc -c $(GCC_FLAGS) msg.c


libplains.o: libplains.c libplains.h msg.o
	gcc -c $(GCC_FLAGS) libplains.c


clean:
	rm -f *.o msg.h msg.c