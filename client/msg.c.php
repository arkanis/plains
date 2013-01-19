<?php

require('protocol.php');

?>
#include <stdio.h>
#include <assert.h>
#include "msg.h"

const char* plains_msg_types[] = {
<?	foreach($messages as $name => $fields): ?>
	"<?= $name ?>",
<?	endforeach ?>
	NULL
};

void plains_msg_print(plains_msg_p msg){
	switch (msg->type){
<?	foreach($messages as $message_name => $fields): ?>
		case PLAINS_MSG_<?= strtoupper($message_name) ?>:
			printf("%hu %s: ", msg->seq, plains_msg_types[msg->type]);
<?			foreach($fields as $name => $type): ?>
<?				if ( is_array($type) ): ?>
<?					if ( $type[0] == 'char' ): ?>
			printf(<?= '"' . $name . ': \"%.*s\" "' ?>, (int)msg-><?= $message_name ?>.<?= $name ?>_length, msg-><?= $message_name ?>.<?= $name ?>);
<?					else: ?>
			printf(<?= '"' . $name . ': [ "' ?>);
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
			printf("%hu unknown message type %hu\n", msg->seq, msg->type);
			break;
	}
}

int plains_msg_serialize(plains_msg_p msg, void* msg_buffer, size_t* msg_buffer_size, int* fd_buffer, size_t* fd_buffer_length){
	void* mp = msg_buffer;
	int* fp = fd_buffer;
	void* msg_buffer_end = msg_buffer + *msg_buffer_size;
	int* fd_buffer_end = fd_buffer + *fd_buffer_length;
	
<?	foreach($header as $name => $type): ?>
	if (mp + sizeof(<?= $type ?>) > msg_buffer_end) goto msg_buffer_overflow;
	*((<?= $type ?>*)mp) = msg-><?= $name ?>;
	mp += sizeof(<?= $type ?>);
<?	endforeach ?>
	
	switch (msg->type){
<?	foreach($messages as $message_name => $fields): ?>
		case PLAINS_MSG_<?= strtoupper($message_name) ?>:
<?			foreach($fields as $name => $type): ?>
<?				if ( is_array($type) ): ?>
			if (mp + sizeof(<?= $len_type ?>) > msg_buffer_end) goto msg_buffer_overflow;
			*((<?= $len_type ?>*)mp) = msg-><?= $message_name ?>.<?= $name ?>_length;
			mp += sizeof(<?= $len_type ?>);
<?					if ($type == 'plains_fd_t'): ?>
			{
				size_t fd_count = msg-><?= $message_name ?>.<?= $name ?>_length;
				if (fp + fd_count > fd_buffer_end) goto fd_buffer_overflow;
				for(size_t i = 0; i < fd_count; i++)
					fd[i] = msg-><?= $message_name ?>.<?= $name ?>[i];
				fp += fd_count;
			}
<?					else: ?>
			{
				size_t length_in_bytes = msg-><?= $message_name ?>.<?= $name ?>_length * sizeof(<?= $type[0] ?>);
				if (mp + length_in_bytes > msg_buffer_end) goto msg_buffer_overflow;
				memcpy(mp, msg-><?= $message_name ?>.<?= $name ?>, length_in_bytes);
				mp += length_in_bytes;
			}
<?					endif ?>
<?				else: ?>
<?					if ($type == 'plains_fd_t'): ?>
			if (fp + 1 > fd_buffer_end) goto fd_buffer_overflow;
			*fp = msg-><?= $message_name ?>.<?= $name ?>;
			fp++;
<?					else: ?>
			if (mp + sizeof(<?= $type ?>) > msg_buffer_end) goto msg_buffer_overflow;
			*((<?= $type ?>*)mp) = msg-><?= $message_name ?>.<?= $name ?>;
			mp += sizeof(<?= $type ?>);
<?					endif ?>
<?				endif ?>
<?			endforeach ?>
			break;
<?	endforeach ?>
		default:
			fprintf(stderr, "plains_msg_serialize(): unknown message type %hu\n", msg->type);
			goto ser_error;
	}
	
	*msg_buffer_size = mp - msg_buffer;
	*fd_buffer_length = fp - fd_buffer;
	return 0;
	
	msg_buffer_overflow:
		fprintf(stderr, "plains_msg_serialize(): message buffer overflow\n");
		goto ser_error;
	
	fd_buffer_overflow:
		fprintf(stderr, "plains_msg_serialize(): file descriptor buffer overflow\n");
		goto ser_error;
	
	ser_error:
		*msg_buffer_size = mp - msg_buffer;
		*fd_buffer_length = fp - fd_buffer;
	return -1;
}

int plains_msg_deserialize(plains_msg_p msg, void* msg_buffer, size_t msg_buffer_size, int* fd_buffer, size_t fd_buffer_length){
	void* mp = msg_buffer;
	int* fp = fd_buffer;
	void* msg_buffer_end = msg_buffer + msg_buffer_size;
	int* fd_buffer_end = fd_buffer + fd_buffer_length;
	
<?	foreach($header as $name => $type): ?>
	if (mp + sizeof(<?= $type ?>) > msg_buffer_end) goto msg_buffer_overrun;
	msg-><?= $name ?> = *((<?= $type ?>*)mp);
	mp += sizeof(<?= $type ?>);
<?	endforeach ?>
	
	switch (msg->type){
<?	foreach($messages as $message_name => $fields): ?>
		case PLAINS_MSG_<?= strtoupper($message_name) ?>:
<?			foreach($fields as $name => $type): ?>
<?				if ( is_array($type) ): ?>
			if (mp + sizeof(<?= $len_type ?>) > msg_buffer_end) goto msg_buffer_overrun;
			msg-><?= $message_name ?>.<?= $name ?>_length = *((<?= $len_type ?>*)mp);
			mp += sizeof(<?= $len_type ?>);
<?					if ($type == 'plains_fd_t'): ?>
			{
				size_t fd_count = msg-><?= $message_name ?>.<?= $name ?>_length;
				if (fp + fd_count > fd_buffer_end) goto fd_buffer_overrun;
				msg-><?= $message_name ?>.<?= $name ?> = fp;
				fp += fd_count;
			}
<?					else: ?>
			{
				size_t length_in_bytes = msg-><?= $message_name ?>.<?= $name ?>_length * sizeof(<?= $type[0] ?>);
				if (mp + length_in_bytes > msg_buffer_end) goto msg_buffer_overrun;
				msg-><?= $message_name ?>.<?= $name ?> = mp;
				mp += length_in_bytes;
			}
<?					endif ?>
<?				else: ?>
<?					if ($type == 'plains_fd_t'): ?>
			if (fp + 1 > fd_buffer_end) goto fd_buffer_overrun;
			msg-><?= $message_name ?>.<?= $name ?> = *fp;
			fp++;
<?					else: ?>
			if (mp + sizeof(<?= $type ?>) > msg_buffer_end) goto msg_buffer_overrun;
			msg-><?= $message_name ?>.<?= $name ?> = *((<?= $type ?>*)mp);
			mp += sizeof(<?= $type ?>);
<?					endif ?>
<?				endif ?>
<?			endforeach ?>
			break;
<?	endforeach ?>
		default:
			fprintf(stderr, "plains_msg_deserialize(): unknown message type %hu\n", msg->type);
			return -1;
	}
	
	return 1;
	
	msg_buffer_overrun:
	fprintf(stderr, "plains_msg_deserialize(): message buffer overrun\n");
	return -1;
	
	fd_buffer_overrun:
	fprintf(stderr, "plains_msg_deserialize(): file descriptor buffer overrun\n");
	return -1;
}