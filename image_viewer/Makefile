GCC_FLAGS = -std=gnu99 -g -Wall
PLAINS_OPTS = -I../client/include -L../client/link -lplains
MUPDF_LIBS = -lfitz -lfreetype -lopenjpeg -ljbig2dec -ljpeg -lpng -lm -lz

image_viewer: image_viewer.c
	cd ../client; make lib
	gcc $(GCC_FLAGS) image_viewer.c $(PLAINS_OPTS) -lm $(MUPDF_LIBS) -o image_viewer

clean:
	rm -f *.o image_viewer