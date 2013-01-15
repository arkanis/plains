GCC_FLAGS = -std=gnu99 -g -Wall

rect_intersection: rect_intersection.c ../math.c ../math.h
	cd ..; make math.o
	gcc $(GCC_FLAGS) rect_intersection.c ../math.o -o rect_intersection

clean:
	rm -f rect_intersection