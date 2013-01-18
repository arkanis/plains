#pragma once

#include "msg.h"

#define PLAINS_ESERIALIZE   -1
#define PLAINS_EWRITE       -2  // errno set
#define PLAINS_ESENDMSG     -3  // errno set
#define PLAINS_EINCOMPLETE  -4
#define PLAINS_ERECVMSG     -5
#define PLAINS_EDESERIALIZE -6

int plains_msg_send(int socket_fd, plains_msg_p msg, void* msg_buffer, size_t msg_buffer_size, int* fd_buffer, size_t fd_buffer_length);
int plains_msg_receive(int socket_fd, plains_msg_p msg, void* msg_buffer, size_t msg_buffer_size, int* fd_buffer, size_t fd_buffer_length);