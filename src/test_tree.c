#include <stdio.h>
#include "uint64_tree.h"

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
	int iterator(uint64_tree_p n){
		printf("%lu\n", n->value);
		return 0;
	}
	uint64_tree_iterate(tree, iterator);
	
	uint64_tree_destroy(tree);
	
	return 0;
}