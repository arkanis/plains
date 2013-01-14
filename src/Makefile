GCC_FLAGS = -std=gnu99 -g -Wall
PHP_OPTS = -d short_open_tag=on

base: base.c common.o math.o viewport.o layer.o tile_table.o ipc_server.o msg.o msg_queue.o
	gcc $(GCC_FLAGS) base.c *.o -lSDL -lGL -lm -lrt -o base

layer.o: layer.c layer.h common.h viewport.h math.h draw_request_tree.o
	gcc -c $(GCC_FLAGS) layer.c

layer_tree.c layer_tree.h layer_tree.o: tree.php
	php $(PHP_OPTS) tree.php layer_p layer_tree "\"layer.h\""
	gcc -c $(GCC_FLAGS) layer_tree.c

draw_request_tree.c draw_request_tree.h draw_request_tree.o: tree.php
	php $(PHP_OPTS) tree.php draw_request_t draw_request_tree "\"draw_request.h\""
	gcc -c $(GCC_FLAGS) draw_request_tree.c

common.o: common.c common.h
	gcc -c $(GCC_FLAGS) common.c

viewport.o: viewport.c viewport.h math.h
	gcc -c $(GCC_FLAGS) viewport.c

math.o: math.c math.h
	gcc -c $(GCC_FLAGS) math.c

tile_table.o: tile_table.c tile_table.h math.h
	gcc -c $(GCC_FLAGS) tile_table.c

ipc_server.o: ipc_server.c ipc_server.h msg.h msg_queue.h
	gcc -c $(GCC_FLAGS) ipc_server.c

msg_queue.o: msg_queue.c msg_queue.h msg.h
	gcc -c $(GCC_FLAGS) msg_queue.c

msg.c msg.h: msg.php
	php $(PHP_OPTS) msg.php

msg.o: msg.c msg.h
	gcc -c $(GCC_FLAGS) msg.c


libplains.o: libplains.c libplains.h msg.o
	gcc -c $(GCC_FLAGS) libplains.c

image_viewer: image_viewer.c libplains.o
	gcc $(GCC_FLAGS) image_viewer.c msg.o libplains.o -o image_viewer -lm

test_server: test_server.c msg.o msg_queue.o ipc_server.o
	gcc $(GCC_FLAGS) test_server.c msg.o msg_queue.o ipc_server.o -o test_server

test_client: test_client.c libplains.o
	gcc $(GCC_FLAGS) test_client.c msg.o libplains.o -o test_client


uint64_tree.c uint64_tree.h uint64_tree.o: tree.php
	php $(PHP_OPTS) tree.php uint64_t uint64_tree "<stdint.h>"
	gcc -c $(GCC_FLAGS) uint64_tree.c

test_tree: test_tree.c uint64_tree.o
	gcc $(GCC_FLAGS) test_tree.c uint64_tree.o -o test_tree


clean:
	rm -f *.o base core test_server test_client msg.h msg.c uint64_tree.c uint64_tree.h draw_request_tree.c draw_request_tree.h image_viewer