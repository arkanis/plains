GCC_FLAGS = -std=gnu99 -g -Wall
PLAINS_OPTS = -I../client/include -L../client/link -lplains

image_viewer: image_viewer.c
	cd ../client; make lib
	gcc $(GCC_FLAGS) image_viewer.c $(PLAINS_OPTS) -lm -o image_viewer

clean:
	rm -f *.o image_viewer