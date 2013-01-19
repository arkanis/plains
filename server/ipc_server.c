#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/un.h>
#include <sys/socket.h>
#include <ctype.h>
#include <poll.h>
#include <errno.h>
#include <fcntl.h>

#include <plains/net.h>
#include "ipc_server.h"


ipc_client_p alloc_client(ipc_server_p server);
void free_client(ipc_server_p server, size_t client_idx);


ipc_server_p ipc_server_new(const char *path, int backlog){
	ipc_server_p server = malloc(sizeof(ipc_server_t));
	
	server->server_fd = socket(AF_UNIX, SOCK_SEQPACKET, 0);
	if (server->server_fd == -1){
		perror("socket() failed");
		goto server_start_failed;
	}
	
	if (unlink(path) == -1 && errno != ENOENT){
		perror("unlink() failed");
		goto server_start_failed;
	}
	
	struct sockaddr_un addr;
	memset(&addr, 0, sizeof(addr));
	addr.sun_family = AF_UNIX;
	strncpy(addr.sun_path, path, sizeof(addr.sun_path) - 1);
	
	if (bind(server->server_fd, (struct sockaddr *)&addr, sizeof(addr)) == -1){
		perror("bind() failed");
		goto server_start_failed;
	}
	
	if (listen(server->server_fd, backlog) == -1){
		perror("listen() failed");
		goto server_start_failed;
	}
	
	// Make server socket non blocking
	fcntl(server->server_fd, F_SETFL, fcntl(server->server_fd, F_GETFL) | O_NONBLOCK);
	
	server->server_path = strdup(path);
	server->client_count = 0;
	server->clients = NULL;
	return server;
	
	server_start_failed:
		free(server);
		return NULL;
}

// TODO: Add a disconnect handler to clean up private data of the connections?
void ipc_server_destroy(ipc_server_p server){
	for(size_t i = 0; i < server->client_count; i++)
		close(server->clients[i].socket);
	close(server->server_fd);
	unlink(server->server_path);
	
	free(server->clients);
	free(server->server_path);
	free(server);
}


int ipc_server_send(ipc_client_p client, plains_msg_p msg){
	msg->seq = client->seq;
	
	int err = plains_msg_send(client->socket, msg, client->send_msg_buffer, PLAINS_MAX_MSG_SIZE, client->send_fd_buffer, PLAINS_MAX_FD_COUNT);
	switch(err){
		case PLAINS_ESERIALIZE:
			fprintf(stderr, "plains_send(): plains_msg_serialize() failed");
			return -1;
		case PLAINS_EWRITE:
			if (errno == EWOULDBLOCK)
				return -2;
			perror("plains_send(): write() failed");
			return -1;
		case PLAINS_ESENDMSG:
			if (errno == EWOULDBLOCK)
				return -2;
			perror("plains_send(): sendmsg() failed");
			return -1;
		case PLAINS_EINCOMPLETE:
			// Error already printed by plains_msg_send()
			return -1;
	}
	
	client->seq++;
	return err;
}


int ipc_server_broadcast(ipc_server_p server, plains_msg_p msg){
	int dropped_messages = 0;
	for(size_t i = 0; i < server->client_count; i++){
		int err = ipc_server_send(&server->clients[i], msg);
		if (err < 0)
			dropped_messages++;
	}
	
	return dropped_messages;
}

int ipc_server_cycle(ipc_server_p server, int timeout, ipc_server_recv_handler_t recv_handler, ipc_server_connect_handler_t connect_handler, ipc_server_disconnect_handler_t disconnect_handler){
	size_t poll_fd_count = server->client_count + 1;
	struct pollfd poll_fds[poll_fd_count];
	
	// Listen on server socket for new connections
	poll_fds[0] = (struct pollfd){
		.fd = server->server_fd,
		.events = POLLIN,
		.revents = 0
	};
	// Listen on clients for new messages (skip dead clients)
	size_t client_idx = 0;
	for(size_t i = 0; i < server->allocated_client_count; i++){
		if (server->clients[i].socket == -1)
			continue;
		
		poll_fds[client_idx+1] = (struct pollfd){
			.fd = server->clients[i].socket,
			.events = POLLIN,
			.revents = 0
		};
		client_idx++;
	}
	
	if ( poll(poll_fds, poll_fd_count, timeout) == -1 ){
		perror("poll() failed");
		return -1;
	}
	
	
	if (poll_fds[0].revents & POLLIN) {
		// We can accept a new connection
		int client_fd = accept(poll_fds[0].fd, NULL, NULL);
		// Make client socket non blocking
		fcntl(client_fd, F_SETFL, fcntl(client_fd, F_GETFL) | O_NONBLOCK);
		
		ipc_client_p client = alloc_client(server);
		client->socket = client_fd;
		client->seq = 0;
		client->client_private = NULL;
		client->server_private = NULL;
		connect_handler(client);
	} else if (poll_fds[0].revents & POLLERR) {
		// Error on the server socket
		int error = 0;
		socklen_t len = sizeof(error);
		getsockopt(poll_fds[0].fd, SOL_SOCKET, SO_ERROR, &error, &len);
		fprintf(stderr, "server socket error: %s\n", strerror(error));
	}
	
	client_idx = 0;
	for(size_t i = 1; i < poll_fd_count; i++){
		// Skip dead clients as we did when building the poll_fd array
		ipc_client_p client = NULL;
		do {
			client = &server->clients[client_idx];
			client_idx++;
		} while(client->socket == -1);
		
		if (poll_fds[i+1].revents & POLLHUP){
			// Client connection closed, mark him dead and clean up our end
			close(client->socket);
			disconnect_handler(client);
			free_client(server, client_idx);
			// No point of receiving or sending messages to the closed fd, so skip
			// the rest for this fd.
			break;
		}
		
		if (poll_fds[i+1].revents & POLLIN){
			// There are messages to be read, loop until there is nothing more to read
			plains_msg_t msg;
			int err;
			while ( (err = plains_msg_receive(client->socket, &msg, client->receive_msg_buffer, PLAINS_MAX_MSG_SIZE, client->receive_fd_buffer, PLAINS_MAX_FD_COUNT)) ){
				if (err < 1) {
					// Break if connection is closed (err == 0), socket buffer is full (err == PLAINS_ERECVMSG && errno == EWOULDBLOCK)
					// or an error ocurred.
					if (err == PLAINS_EDESERIALIZE)
						fprintf(stderr, "plains_receive(): plains_msg_deserialize() failed");
					else if (err == PLAINS_ERECVMSG && errno != EWOULDBLOCK)
						perror("plains_receive(): recvmsg() failed");
					break;
				}
				
				recv_handler(client, &msg);
			}
		}
	}
	
	return 0;
}

/**
 * Reuses the first dead client in the list or allocates new space if necessary.
 * WARNING: Direct pointers to ipc_client_p structures can become invalid after
 * this call! Only client indices remain valid!
 */
ipc_client_p alloc_client(ipc_server_p server){
	// Search for first dead client and return it
	if (server->allocated_client_count > server->client_count) {
		for(size_t i = 0; i < server->allocated_client_count; i++){
			if (server->clients[i].socket == -1) {
				server->client_count++;
				return &server->clients[i];
			}
		}
	}
	
	// No dead client found... 
	server->client_count++;
	server->allocated_client_count++;
	server->clients = realloc(server->clients, server->allocated_client_count * sizeof(ipc_client_t));
	return &server->clients[server->allocated_client_count - 1];
}

void free_client(ipc_server_p server, size_t client_idx){
	server->clients[client_idx].socket = -1;
	server->client_count--;
}