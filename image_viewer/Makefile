GCC_FLAGS = -std=gnu99 -O3 -Wall
PLAINS_OPTS = -I../client/include -L../client/link -lplains
MUPDF_LIBS = -lfitz -lfreetype -lopenjpeg -ljbig2dec -ljpeg -lpng -lm -lz

MUPDF_ARGS = -Imupdf/fitz -Imupdf/pdf -Lmupdf/build/native

image_viewer: image_viewer.c mupdf
	cd ../client; make lib
	gcc $(GCC_FLAGS) $(MUPDF_ARGS) image_viewer.c $(PLAINS_OPTS) -lm $(MUPDF_LIBS) -o image_viewer
	strip image_viewer

ubuntu_deps:
	sudo apt-get install libfreetype6-dev libjbig2dec0-dev libjpeg62-dev libopenjpeg-dev zlib1g-dev build-essential pkg-config

mupdf:
	git clone --depth 0 git://git.ghostscript.com/mupdf.git
	cd mupdf; git checkout 1.1
	cd mupdf; wget http://mupdf.com/download/mupdf-thirdparty.zip
	cd mupdf; unzip mupdf-thirdparty.zip
	cd mupdf; build=native make -j 2

clean:
	rm -f *.o image_viewer
	rm -rf mupdf