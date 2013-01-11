<?php

$header = [
	'type' => 'uint16_t',
	'seq' => 'uint16_t'
];
$messages = [
	// Handshake
	'hello' => [
		'version' => 'uint8_t',
		'name' => ['char', 'max_len' => 64],
		'caps' => ['uint16_t', 'max_len' => 64]
	],
	'welcome' => [
		'version' => 'uint8_t',
		'name' => ['char', 'max_len' => 64],
		'caps' => ['uint16_t', 'max_len' => 64]
	],
	
	// Common messages
	'status' => [
		'seq' => 'uint16_t',
		'status' => 'uint32_t',
		'id' => 'uint32_t'
	],
	
	// Requests for clients
	'layer_create' => [
		'x' => 'int64_t',
		'y' => 'int64_t',
		'z' => 'int64_t',
		'width' => 'uint64_t',
		'height' => 'uint64_t'
	],
	
	// Events triggered by the server
	'draw' => [
		'layer_id' => 'uint32_t',
		
		'x' => 'int64_t',
		'y' => 'int64_t',
		'z' => 'int64_t',
		'width' => 'uint64_t',
		'height' => 'uint64_t',
		'scale_index' => 'int8_t',
		'scale' => 'float'
	]
];
$len_type = 'size_t';

$types = [
	'int8_t'   => ['size' => 1, 'format' => '%hhd'],
	'int16_t'  => ['size' => 2, 'format' => '%hd'],
	'int32_t'  => ['size' => 4, 'format' => '%d'],
	'int64_t'  => ['size' => 8, 'format' => '%ld'],
	'uint8_t'  => ['size' => 1, 'format' => '%hhu'],
	'uint16_t' => ['size' => 2, 'format' => '%hu'],
	'uint32_t' => ['size' => 4, 'format' => '%u'],
	'uint64_t' => ['size' => 8, 'format' => '%lu'],
	'size_t'   => ['size' => 8, 'format' => '%zu'],
	'int'      => ['size' => 4, 'format' => '%d'],
	'float'    => ['size' => 4, 'format' => '%f'],
	'double'   => ['size' => 8, 'format' => '%lf'],
	'char'     => ['size' => 1, 'format' => '%s']
];


//
// Header file
//

ob_start();
?>
#pragma once

#include <stdint.h>
#include <string.h>

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
	int fd;
} msg_t, *msg_p;

<?
$header_size = 0;
foreach($header as $name => $type)
	$header_size += $types[$type]['size'];

// Find the largest message and use it's size to determine the max size of the buffers
$max_message_size = 0;
foreach($messages as $message_name => $fields){
	$message_size = 0;
	foreach($fields as $name => $type){
		if ( is_array($type) )
			$message_size += $types[$len_type]['size'] + $type['max_len'] * $types[$type[0]]['size'];
		else
			$message_size += $types[$type]['size'];
	}
	$max_message_size = max($max_message_size, $message_size);
}
?>
#define MSG_MAX_SIZE <?= $header_size + $max_message_size ?> 

<? $index = 0 ?>
<?	foreach($messages as $name => $fields): ?>
#define MSG_<?= strtoupper($name) ?>	<?= $index++ ?> 
<?	endforeach ?>

extern const char* msg_types[];

void msg_print(msg_p msg);
ssize_t msg_serialize(msg_p msg, void* buffer, size_t buffer_size);
ssize_t msg_deserialize(msg_p msg, void* buffer, size_t buffer_size);


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
static inline msg_p msg_<?= $message_name ?>(msg_p msg, <?= join(', ', $args) ?>){
	msg->type = MSG_<?= strtoupper($message_name) ?>;
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

<?php
file_put_contents("msg.h", ob_get_clean());


//
// Implementation
//

ob_start();
?>
#include <stdio.h>
#include <assert.h>
#include "msg.h"

const char* msg_types[] = {
<?	foreach($messages as $name => $fields): ?>
	"<?= $name ?>",
<?	endforeach ?>
	NULL
};

void msg_print(msg_p msg){
	switch (msg->type){
<?	foreach($messages as $message_name => $fields): ?>
		case MSG_<?= strtoupper($message_name) ?>:
			printf("%s %hu: ", msg_types[msg->type], msg->seq);
<?			foreach($fields as $name => $type): ?>
<?				if ( is_array($type) ): ?>
<?					if ( $type[0] == 'char' ): ?>
			printf(<?= '"' . $name . ': (' . $types[$len_type]['format'] . ')\"%.*s\" "' ?>, msg-><?= $message_name ?>.<?= $name ?>_length,
				(int)msg-><?= $message_name ?>.<?= $name ?>_length, msg-><?= $message_name ?>.<?= $name ?>);
<?					else: ?>
			printf(<?= '"' . $name . ': (' . $types[$len_type]['format'] . ')[ "' ?>, msg-><?= $message_name ?>.<?= $name ?>_length);
			for(size_t i = 0; i < msg-><?= $message_name ?>.<?= $name ?>_length; i++)
				printf(<?= '"' . $types[$type[0]]['format'] . ' "' ?>, msg-><?= $message_name ?>.<?= $name ?>[i]);
			printf("] ");
<?					endif ?>
<?				else: ?>
			printf(<?= '"' . $name . ': ' . $types[$type]['format'] . ' "' ?>, msg-><?= $message_name ?>.<?= $name ?>);
<?				endif ?>
<?			endforeach ?>
			printf("\n");
			break;
<?	endforeach ?>
		default:
<?
			$format = []; $args = [];
			foreach($header as $name => $type){
				$format[] = $name . ': ' . $types[$type]['format'];
				$args[] = 'msg->' . $name;
			}
?>
			printf(<?= '"unknown message, ' . join(', ', $format) . '\n"' ?>, <?= join(', ', $args) ?>);
			break;
	}
}

ssize_t msg_serialize(msg_p msg, void* buffer, size_t buffer_size){
	void* ptr = buffer;
	
<?	foreach($header as $name => $type): ?>
	assert(ptr + sizeof(<?= $type ?>) <= buffer + buffer_size);
	*((<?= $type ?>*)ptr) = msg-><?= $name ?>;
	ptr += sizeof(<?= $type ?>);
<?	endforeach ?>
	
	switch (msg->type){
<?	foreach($messages as $message_name => $fields): ?>
		case MSG_<?= strtoupper($message_name) ?>:
<?			foreach($fields as $name => $type): ?>
<?				if ( is_array($type) ): ?>
			{
				assert(ptr + sizeof(<?= $len_type ?>) <= buffer + buffer_size);
				*((<?= $len_type ?>*)ptr) = msg-><?= $message_name ?>.<?= $name ?>_length;
				ptr += sizeof(<?= $len_type ?>);
				
				size_t length_in_bytes = msg-><?= $message_name ?>.<?= $name ?>_length * sizeof(<?= $type[0] ?>);
				assert(ptr + length_in_bytes <= buffer + buffer_size);
				memcpy(ptr, msg-><?= $message_name ?>.<?= $name ?>, length_in_bytes);
				ptr += length_in_bytes;
			}
<?				else: ?>
			assert(ptr + sizeof(<?= $type ?>) <= buffer + buffer_size);
			*((<?= $type ?>*)ptr) = msg-><?= $message_name ?>.<?= $name ?>;
			ptr += sizeof(<?= $type ?>);
<?				endif ?>
<?			endforeach ?>
			break;
<?	endforeach ?>
		default:
			assert(0);
			break;
	}
	
	return ptr - buffer;
}

ssize_t msg_deserialize(msg_p msg, void* buffer, size_t buffer_size){
	void* ptr = buffer;
	
<?	foreach($header as $name => $type): ?>
	assert(ptr + sizeof(<?= $type ?>) <= buffer + buffer_size);
	msg-><?= $name ?> = *((<?= $type ?>*)ptr);
	ptr += sizeof(<?= $type ?>);
<?	endforeach ?>
	
	switch (msg->type){
<?	foreach($messages as $message_name => $fields): ?>
		case MSG_<?= strtoupper($message_name) ?>:
<?			foreach($fields as $name => $type): ?>
<?				if ( is_array($type) ): ?>
			{
				assert(ptr + sizeof(<?= $len_type ?>) <= buffer + buffer_size);
				msg-><?= $message_name ?>.<?= $name ?>_length = *((<?= $len_type ?>*)ptr);
				ptr += sizeof(<?= $len_type ?>);
				
				size_t length_in_bytes = msg-><?= $message_name ?>.<?= $name ?>_length * sizeof(<?= $type[0] ?>);
				assert(ptr + length_in_bytes <= buffer + buffer_size);
				msg-><?= $message_name ?>.<?= $name ?> = ptr;
				ptr += length_in_bytes;
			}
<?				else: ?>
			assert(ptr + sizeof(<?= $type ?>) <= buffer + buffer_size);
			msg-><?= $message_name ?>.<?= $name ?> = *((<?= $type ?>*)ptr);
			ptr += sizeof(<?= $type ?>);
<?				endif ?>
<?			endforeach ?>
			break;
<?	endforeach ?>
		default:
			assert(0);
			break;
	}
	
	return ptr - buffer;
}
<?php
file_put_contents("msg.c", ob_get_clean());

?>