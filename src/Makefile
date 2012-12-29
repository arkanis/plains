GCC_FLAGS = -std=gnu99 -g

base: base.c common.o math.o viewport.o layer.o tile_table.o
	gcc $(GCC_FLAGS) base.c common.o math.o viewport.o layer.o tile_table.o -lSDL -lGL -o base

layer.o: layer.c layer.h common.h viewport.h math.h
	gcc -c $(GCC_FLAGS) layer.c

common.o: common.c common.h
	gcc -c $(GCC_FLAGS) common.c

viewport.o: viewport.c viewport.h math.h
	gcc -c $(GCC_FLAGS) viewport.c

math.o: math.c math.h
	gcc -c $(GCC_FLAGS) math.c

tile_table.o: tile_table.c tile_table.h math.h
	gcc -c $(GCC_FLAGS) tile_table.c

ipc_server.o: ipc_server.c ipc_server.h msg.h
	gcc -c $(GCC_FLAGS) ipc_server.c

msg_queue.o: msg_queue.c msg_queue.h msg.h
	gcc -c $(GCC_FLAGS) msg_queue.c

msg.c msg.h: msg.php
	php msg.php

msg.o: msg.c msg.h
	gcc -c $(GCC_FLAGS) msg.c


libplains.o: libplains.c libplains.h msg.o
	gcc -c $(GCC_FLAGS) libplains.c

test_server: test_server.c msg.o msg_queue.o ipc_server.o
	gcc $(GCC_FLAGS) test_server.c msg.o msg_queue.o ipc_server.o -o test_server

test_client: test_client.c libplains.o
	gcc $(GCC_FLAGS) test_client.c msg.o libplains.o -o test_client


clean:
	rm -f *.o base core test_server test_client