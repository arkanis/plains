GCC_FLAGS = -std=gnu99 -g

base: base.c common.o math.o viewport.o
	gcc $(GCC_FLAGS) base.c common.o math.o viewport.o -lSDL -lGL -o base

common.o: common.c common.h
	gcc -c $(GCC_FLAGS) common.c

viewport.o: viewport.c viewport.h math.h
	gcc -c $(GCC_FLAGS) viewport.c

math.o: math.c math.h
	gcc -c $(GCC_FLAGS) math.c

clean:
	rm -f *.o base core