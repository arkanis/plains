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

clean:
	rm -f *.o base core