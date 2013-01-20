GCC_FLAGS = -std=gnu99 -O3 -Wall
PHP_OPTS = -d short_open_tag=on

LIB_OBJS = client.o msg.o net.o
lib: link include

link: $(LIB_OBJS)
	mkdir -p link
	ar rcs link/libplains.a $(LIB_OBJS)

include: $(LIB_OBJS)
	mkdir -p include/plains
	cp *.h include/plains

client.o: client.c client.h msg.o net.o
	gcc -c $(GCC_FLAGS) client.c

msg.h: msg.h.php protocol.php
	php $(PHP_OPTS) msg.h.php > msg.h

msg.c: msg.c.php msg.h
	php $(PHP_OPTS) msg.c.php > msg.c

msg.o: msg.c msg.h
	gcc -c $(GCC_FLAGS) msg.c

net.o: net.c net.h msg.h
	gcc -c $(GCC_FLAGS) net.c


clean:
	rm -f *.o msg.h msg.c
	rm -rf link include
	cd tests; make clean