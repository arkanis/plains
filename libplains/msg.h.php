<?php

require('protocol.php');

// Calculate maximal size of message and fd buffer
$header_size = 0;
foreach($header as $name => $type)
	$header_size += $types[$type]['size'];

// Find the largest message and use it's size to determine the max size of the buffers
$max_msg_buffer_size = 0;
$max_fd_buffer_size = 0;
foreach($messages as $message_name => $fields){
	// Add the header size to the size of each message when finding the largest one
	$msg_buffer_size = $header_size;
	$fd_buffer_size = 0;
	foreach($fields as $name => $type){
		if ( is_array($type) ) {
			$buffer_var = ($type == 'plains_fd_t') ? 'fd_buffer_size' : 'msg_buffer_size';
			$$buffer_var += $types[$len_type]['size'] + $type['max_len'] * $types[$type[0]]['size'];
		} else {
			$buffer_var = ($type == 'plains_fd_t') ? 'fd_buffer_size' : 'msg_buffer_size';
			$$buffer_var += $types[$type]['size'];
		}
	}
	$max_msg_buffer_size = max($max_msg_buffer_size, $msg_buffer_size);
	$max_fd_buffer_size = max($max_fd_buffer_size, $fd_buffer_size);
}

ob_start();
?>
#pragma once

#include <stdint.h>
#include <stddef.h>
//#include <sys/types.h>
#include <string.h>

typedef int plains_fd_t;

typedef struct {
<?	foreach($header as $name => $type): ?>
	<?= $type ?> <?= $name ?>;
<?	endforeach ?>
	union {
<?	foreach($messages as $message_name => $fields): ?>
		struct {
<?			foreach($fields as $name => $type): ?>
<?			if( is_array($type) ): ?>
			<?= $len_type ?> <?= $name ?>_length;
			<?= $type[0] ?> *<?= $name ?>;
<?			else: ?>
			<?= $type ?> <?= $name ?>;
<?			endif ?>
<?			endforeach ?>
		} <?= $message_name ?>;
<?	endforeach ?>
	};
} plains_msg_t, *plains_msg_p;

#define PLAINS_MAX_MSG_SIZE <?= $max_msg_buffer_size ?> 
#define PLAINS_MAX_FD_COUNT <?= $max_fd_buffer_size / $types['plains_fd_t']['size'] ?> 

<? $index = 0 ?>
<?	foreach($messages as $name => $fields): ?>
#define PLAINS_MSG_<?= strtoupper($name) ?>	<?= $index++ ?> 
<?	endforeach ?>

extern const char* plains_msg_types[];

void plains_msg_print(plains_msg_p msg);
int plains_msg_serialize(plains_msg_p msg, void* msg_buffer, size_t* msg_buffer_size, int* fd_buffer, size_t* fd_buffer_size);
int plains_msg_deserialize(plains_msg_p msg, void* msg_buffer, size_t msg_buffer_size, int* fd_buffer, size_t fd_buffer_size);


<?	foreach($messages as $message_name => $fields): ?>
<?
		$args = [];
		foreach($fields as $name => $type){
			if ( is_array($type) ) {
				if ( $type[0] == 'char' ) {
					$args[] = 'char* ' . $name;
				} else {
					$args[] = $type[0] . '* ' . $name;
					$args[] = $len_type . ' ' . $name . '_length';
				}
			} else {
				$args[] = $type . ' ' . $name;
			}
		}
?>
static inline plains_msg_p msg_<?= $message_name ?>(plains_msg_p msg, <?= join(', ', $args) ?>){
	msg->type = PLAINS_MSG_<?= strtoupper($message_name) ?>;
	msg->seq = 0;
	
<?	foreach($fields as $name => $type): ?>
<?		if ( is_array($type) ): ?>
<?			if ( $type[0] == 'char' ): ?>
	msg-><?= $message_name ?>.<?= $name ?> = <?= $name ?>;
	msg-><?= $message_name ?>.<?= $name ?>_length = strlen(<?= $name ?>);
<?			else: ?>
	msg-><?= $message_name ?>.<?= $name ?> = <?= $name ?>;
	msg-><?= $message_name ?>.<?= $name ?>_length = <?= $name ?>_length;
<?			endif ?>
<?		else: ?>
	msg-><?= $message_name ?>.<?= $name ?> = <?= $name ?>;
<?		endif ?>
<?	endforeach ?>
	
	return msg;
}
<?	endforeach ?>