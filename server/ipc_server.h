#pragma once

#include <plains/msg.h>


typedef struct {
	int socket;
	uint16_t seq;
	
	uint8_t send_msg_buffer[PLAINS_MAX_MSG_SIZE];
	int send_fd_buffer[PLAINS_MAX_FD_COUNT];
	uint8_t receive_msg_buffer[PLAINS_MAX_MSG_SIZE];
	int receive_fd_buffer[PLAINS_MAX_FD_COUNT];
	
	void* client_private;
	void* server_private;
} ipc_client_t, *ipc_client_p;

typedef struct {
	int server_fd;
	char *server_path;
	size_t client_count;
	size_t allocated_client_count;
	ipc_client_p clients;
} ipc_server_t, *ipc_server_p;


ipc_server_p ipc_server_new(const char *path, int backlog);
void ipc_server_destroy(ipc_server_p server);

int ipc_server_send(ipc_client_p client, plains_msg_p msg);
int ipc_server_broadcast(ipc_server_p server, plains_msg_p msg);

typedef void (*ipc_server_connect_handler_t)(size_t client_idx, ipc_client_p client);
typedef void (*ipc_server_recv_handler_t)(size_t client_idx, ipc_client_p client, plains_msg_p msg);
typedef void (*ipc_server_disconnect_handler_t)(size_t client_idx, ipc_client_p client);
int ipc_server_cycle(ipc_server_p server, int timeout, ipc_server_recv_handler_t recv_handler, ipc_server_connect_handler_t connect_handler, ipc_server_disconnect_handler_t disconnect_handler);