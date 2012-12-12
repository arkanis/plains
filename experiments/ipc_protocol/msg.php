<?php

$messages = [
	// Handshake
	'hello' => [
		'version' => 'uint8_t',
		'name' => 'string'
	],
	'welcome' => [
		'version' => 'uint8_t',
		'name' => 'string'
		// later: caps bit map
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
		'scale' => 'float',
		
		'shm_id' => 'int'
	]
];
$len_type = 'size_t';


//
// Header file
//

ob_start();
?>
#pragma once

#include <stdint.h>

typedef struct {
	uint16_t type;
	uint16_t seq;
	union {
<?	foreach($messages as $message_name => $fields): ?>
		struct {
<?			foreach($fields as $name => $type): ?>
<?			if($type == 'string'): ?>
			char* <?= $name ?>;
<?			else: ?>
			<?= $type ?> <?= $name ?>;
<?			endif ?>
<?			endforeach ?>
		} <?= $message_name ?>;
<?	endforeach ?>
	};
} msg_t, *msg_p;

#define MSG_MAX_SIZE 4096

<? $index = 0 ?>
<?	foreach($messages as $name => $fields): ?>
#define MSG_<?= strtoupper($name) ?>	<?= $index++ ?> 
<?	endforeach ?>

extern const char* msg_types[];

void msg_print(msg_p msg);
ssize_t msg_serialize(msg_p msg, void* buffer, size_t buffer_size);
ssize_t msg_deserialize(msg_p msg, void* buffer, size_t buffer_size);


<?	foreach($messages as $message_name => $fields): ?>
<?		$args = [] ?>
<?		foreach($fields as $name => $type): ?>
<?			$args[] = ($type == 'string' ? 'char*' : $type) . ' ' . $name; ?>
<?		endforeach ?>
static inline msg_p msg_<?= $message_name ?>(<?= join(', ', $args) ?>, msg_p msg){
	msg->type = MSG_<?= strtoupper($message_name) ?>;
	msg->seq = 0;
	
<?	foreach($fields as $name => $type): ?>
	msg-><?= $message_name ?>.<?= $name ?> = <?= $name ?>;
<?	endforeach ?>
	
	return msg;
}
<?	endforeach ?>

<?php
file_put_contents("msg.h", ob_get_clean());


//
// Implementation
//

$printf_types = [
	'int8_t' => '%hhd', 'int16_t' => '%hd', 'int32_t' => '%d', 'int64_t' => '%ld',
	'uint8_t' => '%hhu', 'uint16_t' => '%hu', 'uint32_t' => '%u', 'uint64_t' => '%lu',
	'int' => '%d', 'float' => '%f', 'double' => '%lf', 'string' => '%s'
];


ob_start();
?>
#include <stdio.h>
#include <assert.h>
#include <string.h>
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
<?			$format = [] ?>
<?			$args = [] ?>
<?			foreach($fields as $name => $type): ?>
<?				$format[] = sprintf('%s: %s', $name, $printf_types[$type]) ?>
<?				$args[] = sprintf('msg->%s.%s', $message_name, $name) ?>
<?			endforeach ?>
			printf("%s(%hu): ", msg_types[msg->type], msg->seq);
			printf(<?= '"'.join(', ', $format).'\n"' ?>, <?= join(', ', $args) ?>);
			break;
<?	endforeach ?>
		default:
			printf("unknown message, type: %hhu, seq: %hu\n", msg->type, msg->seq);
			break;
	}
}

ssize_t msg_serialize(msg_p msg, void* buffer, size_t buffer_size){
	void* ptr = buffer;
	
<?	foreach(['type' => 'uint16_t', 'seq' => 'uint16_t'] as $name => $type): ?>
	assert(ptr + sizeof(<?= $type ?>) <= buffer + buffer_size);
	*((<?= $type ?>*)ptr) = msg-><?= $name ?>;
	ptr += sizeof(<?= $type ?>);
<?	endforeach ?>
	
	switch (msg->type){
<?	foreach($messages as $message_name => $fields): ?>
		case MSG_<?= strtoupper($message_name) ?>:
<?			foreach($fields as $name => $type): ?>
<?				if ($type == 'string'): ?>
			{
				<?= $len_type ?> len = strlen(msg-><?= $message_name ?>.<?= $name ?>) + 1;
				assert(ptr + sizeof(<?= $len_type ?>) <= buffer + buffer_size);
				*((<?= $len_type ?>*)ptr) = len;
				ptr += sizeof(<?= $len_type ?>);
				
				assert(ptr + len <= buffer + buffer_size);
				memcpy(ptr, msg-><?= $message_name ?>.<?= $name ?>, len);
				ptr += len;
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
	
<?	foreach(['type' => 'uint16_t', 'seq' => 'uint16_t'] as $name => $type): ?>
	assert(ptr + sizeof(<?= $type ?>) <= buffer + buffer_size);
	msg-><?= $name ?> = *((<?= $type ?>*)ptr);
	ptr += sizeof(<?= $type ?>);
<?	endforeach ?>
	
	switch (msg->type){
<?	foreach($messages as $message_name => $fields): ?>
		case MSG_<?= strtoupper($message_name) ?>:
<?			foreach($fields as $name => $type): ?>
<?				if ($type == 'string'): ?>
			{
				assert(ptr + sizeof(<?= $len_type ?>) <= buffer + buffer_size);
				<?= $len_type ?> len = *((<?= $len_type ?>*)ptr);
				ptr += sizeof(<?= $len_type ?>);
				
				assert(ptr + len <= buffer + buffer_size);
				msg-><?= $message_name ?>.<?= $name ?> = ptr;
				ptr += len;
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