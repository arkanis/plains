#pragma once

#include "msg.h"


typedef struct {
	size_t length, start_idx, end_idx;
	void* buffers;
	size_t* buffer_sizes;
	int* fds;
} msg_queue_t, *msg_queue_p;


msg_queue_t msg_queue_new(size_t length);
void msg_queue_destroy(msg_queue_p queue);

void* msg_queue_start_enqueue(msg_queue_p queue);
void msg_queue_end_enqueue(msg_queue_p queue, void* buffer, size_t buffer_size, int fd);

void* msg_queue_start_dequeue(msg_queue_p queue, size_t* buffer_size, int* fd);
void msg_queue_end_dequeue(msg_queue_p queue, void* buffer);

size_t msg_queue_full_buffers(msg_queue_p queue);
size_t msg_queue_free_buffers(msg_queue_p queue);