#pragma once

#include "msg.h"
#include "msg_queue.h"


typedef struct {
	int fd;
	msg_queue_t in, out;
	uint16_t seq;
	void* private;
} ipc_client_t, *ipc_client_p;

typedef struct {
	int server_fd;
	char *server_path;
	size_t queue_length;
	ssize_t client_connected_idx;
	void *client_disconnected_private;
	size_t client_count;
	ipc_client_p clients;
} ipc_server_t, *ipc_server_p;


ipc_server_p ipc_server_new(const char *path, int backlog, size_t queue_length);
void ipc_server_destroy(ipc_server_p server);

int ipc_server_send(ipc_server_p server, size_t client_idx, msg_p msg);
int ipc_server_send_with_fd(ipc_server_p server, size_t client_idx, msg_p msg, int fd);
int ipc_server_broadcast(ipc_server_p server, msg_p msg);
void ipc_server_cycle(ipc_server_p server, int timeout);