#include <stdio.h>
#include <stdbool.h>
#include <assert.h>
#include "uint64_tree.h"

void test_remove_first(){
	uint64_tree_p tree = uint64_tree_new(0);
	uint64_tree_p n0 = uint64_tree_append(tree, 84);
	uint64_tree_p n1 = uint64_tree_append(tree, 72);
	uint64_tree_p n2 = uint64_tree_append(tree, 32);
	
	uint64_tree_remove(n0);
	assert(tree->first == n1);
	assert(tree->last == n2);
	assert(n1->prev == NULL);
	assert(n1->next == n2);
}

void test_remove_last(){
	uint64_tree_p tree = uint64_tree_new(0);
	uint64_tree_p n0 = uint64_tree_append(tree, 84);
	uint64_tree_p n1 = uint64_tree_append(tree, 72);
	uint64_tree_p n2 = uint64_tree_append(tree, 32);
	
	uint64_tree_remove(n2);
	assert(tree->first == n0);
	assert(tree->last == n1);
	assert(n1->next == NULL);
	assert(n1->prev == n0);
}

void test_remove_middle(){
	uint64_tree_p tree = uint64_tree_new(0);
	uint64_tree_p n0 = uint64_tree_append(tree, 84);
	uint64_tree_p n1 = uint64_tree_append(tree, 72);
	uint64_tree_p n2 = uint64_tree_append(tree, 32);
	
	uint64_tree_remove(n1);
	assert(tree->first == n0);
	assert(tree->last == n2);
	assert(n0->prev == NULL);
	assert(n0->next == n2);
	assert(n2->prev == n0);
	assert(n2->next == NULL);
}

int main(){
	uint64_tree_p tree = uint64_tree_new(0);
	uint64_tree_append(tree, 0);
	uint64_tree_p x = uint64_tree_append(tree, 8);
	uint64_tree_append(tree, 15);
	uint64_tree_prepend(tree, 7);
	
	for(uint64_tree_p node = tree->first; node; node = node->next)
		printf("%lu\n", node->value);
	
	
	printf("Hierarchy:\n");
	
	uint64_tree_append(x, 84);
	uint64_tree_append(x, 72);
	uint64_tree_p y = uint64_tree_append(x, 32);
	uint64_tree_append(x, 99);
	
	uint64_tree_append(y, 3);
	uint64_tree_append(y, 9);
	
	void tree_printer(uint64_tree_p tree, uint8_t depth){
		for(uint64_tree_p node = tree->first; node; node = node->next){
			for(size_t i = 0; i < depth; i++)
				printf("  ");
			printf("%lu\n", node->value);
			tree_printer(node, depth + 1);
		}
	}
	tree_printer(tree, 0);
	
	printf("Collapsed:\n");
	uint64_tree_collapse(x);
	tree_printer(tree, 0);
	
	printf("Iterate:\n");
	uint64_tree_p iterator(uint64_tree_p n){
		printf("%lu\n", n->value);
		return NULL;
	}
	uint64_tree_iterate(tree, iterator);
	
	
	printf("Selectively deleted:\n");
	uint8_t cleaner(uint64_tree_p n){
		if (n->value == 0 || n->value == 99)
			return true;
		return false;
	}
	uint64_tree_delete(tree, cleaner);
	tree_printer(tree, 0);
	
	uint64_tree_destroy(tree);
	
	test_remove_first();
	test_remove_last();
	test_remove_middle();
	
	return 0;
}