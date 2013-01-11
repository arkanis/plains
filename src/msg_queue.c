#include <stdlib.h>
#include <assert.h>
#include "msg_queue.h"

msg_queue_t msg_queue_new(size_t length){
	return (msg_queue_t){
		.length = length, .start_idx = 0, .end_idx = 0,
		.buffers = malloc(MSG_MAX_SIZE * length),
		.buffer_sizes = malloc(sizeof(size_t) * length),
		.fds = malloc(sizeof(int) * length)
	};
}

void msg_queue_destroy(msg_queue_p queue){
	free(queue->buffers);
	free(queue->buffer_sizes);
	free(queue->fds);
	queue->length = 0;
	queue->start_idx = 0;
	queue->end_idx = 0;
}

/**

start  | (0)
end    | (0)
       |--------------------|
full: 0 - 0 = 0

start    | (2)
end          | (6)
       |--------------------|
full: 6 - 2 = 4

start               | (13)
end        | (4)
       |--------------------|
full: length - 13 + 4 = 11 (with length of 20)

**/
size_t msg_queue_full_buffers(msg_queue_p queue){
	if (queue->end_idx >= queue->start_idx)
		return queue->end_idx - queue->start_idx;
	return queue->length - queue->start_idx + queue->end_idx;
}

size_t msg_queue_free_buffers(msg_queue_p queue){
	return queue->length - msg_queue_full_buffers(queue);
}


void* msg_queue_start_enqueue(msg_queue_p queue){
	if ( msg_queue_free_buffers(queue) == 0 )
		return NULL;
	return queue->buffers + queue->end_idx * MSG_MAX_SIZE;
}

void msg_queue_end_enqueue(msg_queue_p queue, void* buffer, size_t buffer_size, int fd){
	size_t incomming_end_idx = (buffer - queue->buffers) / MSG_MAX_SIZE;
	assert(incomming_end_idx == queue->end_idx);
	assert(buffer_size < MSG_MAX_SIZE);
	queue->buffer_sizes[incomming_end_idx] = buffer_size;
	queue->fds[incomming_end_idx] = fd;
	
	// Calculate next end_idx
	queue->end_idx++;
	if (queue->end_idx >= queue->length)
		queue->end_idx -= queue->length;
}

void* msg_queue_start_dequeue(msg_queue_p queue, size_t* buffer_size, int* fd){
	if ( msg_queue_full_buffers(queue) == 0 )
		return NULL;
	if ( buffer_size != NULL )
		*buffer_size = queue->buffer_sizes[queue->start_idx];
	if ( fd != NULL )
		*fd = queue->fds[queue->start_idx];
	return queue->buffers + queue->start_idx * MSG_MAX_SIZE;
}

void msg_queue_end_dequeue(msg_queue_p queue, void* buffer){
	size_t incomming_start_idx = (buffer - queue->buffers) / MSG_MAX_SIZE;
	assert(incomming_start_idx == queue->start_idx);
	
	// Calculate next start_idx
	queue->start_idx++;
	if (queue->start_idx >= queue->length)
		queue->start_idx -= queue->length;
}