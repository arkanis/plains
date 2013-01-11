<?php

$type = $argv[1];
$name = $argv[2];
$header = isset($argv[3]) ? $argv[3] : null;

ob_start();
?>
#pragma once

<? if($header): ?>
#include <?= $header ?>
<? endif ?>

typedef struct <?= $name ?>_s <?= $name ?>_t, *<?= $name ?>_p;

struct <?= $name ?>_s {
	<?= $name ?>_p parent, prev, next;
	<?= $name ?>_p first, last;
	<?= $type ?> value;
};

<?= $name ?>_p <?= $name ?>_new(<?= $type ?> value);
void <?= $name ?>_destroy(<?= $name ?>_p node);

// Same level
<?= $name ?>_p <?= $name ?>_insert_after(<?= $name ?>_p node, <?= $type ?> value);
<?= $name ?>_p <?= $name ?>_insert_before(<?= $name ?>_p node, <?= $type ?> value);
<?= $type ?> <?= $name ?>_remove(<?= $name ?>_p node);

// For children
<?= $name ?>_p <?= $name ?>_append(<?= $name ?>_p node, <?= $type ?> value);
<?= $name ?>_p <?= $name ?>_prepend(<?= $name ?>_p node, <?= $type ?> value);

// Special stuff
void <?= $name ?>_collapse(<?= $name ?>_p node);

typedef int (*<?= $name ?>_iterator_t)(<?= $name ?>_p node);
<?= $name ?>_p <?= $name ?>_iterate(<?= $name ?>_p node, <?= $name ?>_iterator_t func);

<?php
file_put_contents("$name.h", ob_get_clean());
ob_start();
?>
#include <stdlib.h>
#include "<?= $name ?>.h"

<?= $name ?>_p <?= $name ?>_new(<?= $type ?> value){
	<?= $name ?>_p node = malloc(sizeof(<?= $name ?>_t));
	node->parent = NULL;
	node->prev = NULL;
	node->next = NULL;
	node->first = NULL;
	node->last = NULL;
	node->value = value;
	return node;
}

void <?= $name ?>_destroy(<?= $name ?>_p node){
	if (node == NULL)
		return;
	
	for(<?= $name ?>_p child = node->first; child; child = child->next)
		<?= $name ?>_destroy(child);
	free(node);
}

// Same level
<?= $name ?>_p <?= $name ?>_insert_after(<?= $name ?>_p node, <?= $type ?> value){
	<?= $name ?>_p new_node = malloc(sizeof(<?= $name ?>_t));
	*new_node = (<?= $name ?>_t){
		.parent = node->parent,
		.prev = node, .next = node->next,
		.first = NULL, .last = NULL,
		.value = value
	};
	node->next = new_node;
	
	if (new_node->parent){
		if (new_node->prev == NULL)
			new_node->parent->first = new_node;
		if (new_node->next == NULL)
			new_node->parent->last = new_node;
	}
	
	return new_node;
}

<?= $name ?>_p <?= $name ?>_insert_before(<?= $name ?>_p node, <?= $type ?> value){
	<?= $name ?>_p new_node = malloc(sizeof(<?= $name ?>_t));
	*new_node = (<?= $name ?>_t){
		.parent = node->parent,
		.prev = node->prev, .next = node,
		.first = NULL, .last = NULL,
		.value = value
	};
	node->prev = new_node;
	
	if (new_node->parent){
		if (new_node->prev == NULL)
			new_node->parent->first = new_node;
		if (new_node->next == NULL)
			new_node->parent->last = new_node;
	}
	
	return new_node;
}

<?= $type ?> <?= $name ?>_remove(<?= $name ?>_p node){
	<?= $type ?> value = node->value;
	
	if (node->prev == NULL){
		if (node->next)
			node->next->prev = node->prev;
		if (node->parent)
			node->parent->first = node->next;
	}
		
	if (node->next == NULL){
		if (node->prev)
			node->prev->next = node->next;
		if (node->parent)
			node->parent->last = node->prev;
	}
	
	<?= $name ?>_destroy(node);
	
	return value;
}

// For children
<?= $name ?>_p <?= $name ?>_append(<?= $name ?>_p node, <?= $type ?> value){
	<?= $name ?>_p new_node = malloc(sizeof(<?= $name ?>_t));
	*new_node = (<?= $name ?>_t){
		.parent = node,
		.prev = node->last, .next = NULL,
		.first = NULL, .last = NULL,
		.value = value
	};
	if (node->last)
		node->last->next = new_node;
	node->last = new_node;
	
	if (new_node->prev == NULL)
		node->first = new_node;
	
	return new_node;
}

<?= $name ?>_p <?= $name ?>_prepend(<?= $name ?>_p node, <?= $type ?> value){
	<?= $name ?>_p new_node = malloc(sizeof(<?= $name ?>_t));
	*new_node = (<?= $name ?>_t){
		.parent = node,
		.prev = NULL, .next = node->first,
		.first = NULL, .last = NULL,
		.value = value
	};
	if (node->first)
		node->first->prev = new_node;
	node->first = new_node;
	
	if (new_node->next == NULL)
		node->last = new_node;
	
	return new_node;
}

// Special stuff
void <?= $name ?>_collapse(<?= $name ?>_p node){
	for(<?= $name ?>_p child = node->first; child; child = child->next)
		child->parent = node->parent;
	
	node->prev->next = node->first;
	node->first->prev = node->prev;
	
	node->next->prev = node->last;
	node->last->next = node->next;
	
	free(node);
}

<?= $name ?>_p <?= $name ?>_iterate(<?= $name ?>_p node, <?= $name ?>_iterator_t func){
	<?= $name ?>_p result = NULL;
	
	for(<?= $name ?>_p child = node->first; child; child = child->next){
		result = func(child);
		if (result != NULL)
			break;
		
		result = <?= $name ?>_iterate(child, func);
		if (result != NULL)
			break;
	}
	
	return result;
}

<?php
file_put_contents("$name.c", ob_get_clean());
?>