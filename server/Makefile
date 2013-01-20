GCC_FLAGS = -std=gnu99 -O3 -Wall
PHP_OPTS = -d short_open_tag=on

# For parts that depend on client library files
PLAINS_OPTS = -I../client/include -L../client/link -lplains

# Main programs
SERVER_OBJS = viewport.o renderer.o object_tree.o math.o ipc_server.o draw_request_tree.o
server: server.c $(SERVER_OBJS)
	gcc $(GCC_FLAGS) server.c $(SERVER_OBJS) -lSDL -lGL -lm -lrt $(PLAINS_OPTS) -o server

# Parts that depend on the client library
ipc_server.o: ipc_server.c ipc_server.h
	cd ../client; make lib
	gcc -c $(GCC_FLAGS) ipc_server.c $(PLAINS_OPTS)


# Parts
renderer.o: renderer.c renderer.h draw_request.h viewport.o
	gcc -c $(GCC_FLAGS) renderer.c

viewport.o: viewport.c viewport.h math.h object_tree.h
	gcc -c $(GCC_FLAGS) viewport.c


# Collections
object_tree.c object_tree.h object_tree.o: tree.php
	php $(PHP_OPTS) tree.php object_t object_tree "\"object.h\""
	gcc -c $(GCC_FLAGS) object_tree.c

draw_request_tree.c draw_request_tree.h draw_request_tree.o: tree.php
	php $(PHP_OPTS) tree.php draw_request_t draw_request_tree "\"draw_request.h\""
	gcc -c $(GCC_FLAGS) draw_request_tree.c

# Base libraries
math.o: math.c math.h
	gcc -c $(GCC_FLAGS) math.c

clean:
	rm -f `tr '\n' ' ' < .gitignore`
	cd tests; make clean