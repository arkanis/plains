GCC_FLAGS = -std=gnu99 -g -Wall
PHP_OPTS = -d short_open_tag=on

rect_intersection_test: rect_intersection_test.c ../math.c ../math.h
	cd ..; make math.o
	gcc $(GCC_FLAGS) rect_intersection_test.c ../math.o -o rect_intersection_test


tree_test: tree_test.c uint64_tree.o
	gcc $(GCC_FLAGS) tree_test.c uint64_tree.o -o tree_test

uint64_tree.c uint64_tree.h uint64_tree.o: ../tree.php
	php $(PHP_OPTS) ../tree.php uint64_t uint64_tree "<stdint.h>"
	gcc -c $(GCC_FLAGS) uint64_tree.c


clean:
	rm -f *.o *_test uint64_tree.c uint64_tree.h