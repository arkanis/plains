#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/un.h>
#include <sys/socket.h>
#include <ctype.h>
#include <poll.h>
#include <errno.h>
#include <fcntl.h>

#include "ipc_server.h"


void on_client_connect(ipc_server_p server, int client_fd);
void on_client_disconnect(ipc_server_p server, size_t client_idx);


ipc_server_p ipc_server_new(const char *path, int backlog, size_t queue_length){
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
	
	server->queue_length = queue_length;
	server->server_path = strdup(path);
	server->client_count = 0;
	server->clients = NULL;
	return server;
	
	server_start_failed:
		free(server);
		return NULL;
}

void ipc_server_destroy(ipc_server_p server){
	for(size_t i = 0; i < server->client_count; i++)
		close(server->clients[i].fd);
	close(server->server_fd);
	unlink(server->server_path);
	
	free(server->clients);
	free(server->server_path);
	free(server);
}


int ipc_server_send(ipc_server_p server, size_t client_idx, msg_p msg){
	msg_queue_p out = &server->clients[client_idx].out;
	void* buffer = msg_queue_start_enqueue(out);
	if (buffer == NULL)
		return 1;
	
	size_t buffer_filled = msg_serialize(msg, buffer, MSG_MAX_SIZE);
	msg_queue_end_enqueue(out, buffer, buffer_filled);
	return 0;
}

int ipc_server_broadcast(ipc_server_p server, msg_p msg){
	int dropped_messages = 0;
	for(size_t i = 0; i < server->client_count; i++){
		msg_queue_p out = &server->clients[i].out;
		void* buffer = msg_queue_start_enqueue(out);
		if (buffer == NULL){
			dropped_messages++;
			continue;
		}
		
		size_t buffer_filled = msg_serialize(msg, buffer, MSG_MAX_SIZE);
		msg_queue_end_enqueue(out, buffer, buffer_filled);
	}
	
	return dropped_messages;
}

void ipc_server_cycle(ipc_server_p server, int timeout){
	server->client_connected_idx = -1;
	server->client_disconnected_private = NULL;
	
	size_t poll_fd_count = server->client_count + 1;
	struct pollfd poll_fds[poll_fd_count];
	
	poll_fds[0] = (struct pollfd){
		.fd = server->server_fd,
		.events = POLLIN,
		.revents = 0
	};
	for(size_t i = 0; i < server->client_count; i++){
		poll_fds[i+1] = (struct pollfd){
			.fd = server->clients[i].fd,
			.events = 0,
			.revents = 0
		};
		if ( msg_queue_full_buffers(&server->clients[i].out) > 0 )
			poll_fds[i+1].events |= POLLOUT;
		if ( msg_queue_free_buffers(&server->clients[i].in) > 0 )
			poll_fds[i+1].events |= POLLIN;
	}
	
	if (poll(poll_fds, poll_fd_count, timeout) == -1){
		perror("poll() failed");
	}
	
	if (poll_fds[0].revents & POLLIN) {
		// We can accept a new connection
		int client_fd = accept(poll_fds[0].fd, NULL, NULL);
		on_client_connect(server, client_fd);
		return;
	} else if (poll_fds[0].revents & POLLERR) {
		// Error on the server socket
		int error = 0;
		socklen_t len = sizeof(error);
		getsockopt(poll_fds[0].fd, SOL_SOCKET, SO_ERROR, &error, &len);
		fprintf(stderr, "server socket error: %s\n", strerror(error));
	}
	
	for(size_t i = 0; i < server->client_count; i++){
		if (poll_fds[i+1].revents & POLLHUP){
			// Client connection closed
			on_client_disconnect(server, i);
			// No point of receiving or sending messages to the closed fd, so skip
			// the rest. Also let someone outside clean up the client private data.
			// Therefore abort the entire poll stuff.
			return;
		}
		
		if (poll_fds[i+1].revents & POLLIN){
			// There are messages to be read, loop until there is nothing more to read
			// or we run out of free buffers.
			void* buffer = NULL;
			while( (buffer = msg_queue_start_enqueue(&server->clients[i].in)) != NULL ){
				ssize_t bytes_read = read(poll_fds[i+1].fd, buffer, MSG_MAX_SIZE);
				
				// Break the read loop on an unexpected EOF
				if (bytes_read == 0)
					break;
				
				// Break the read loop on failure and report it if its an error unconcerned
				// to non blocking IO.
				if (bytes_read == -1){
					if (errno != EWOULDBLOCK && errno != EAGAIN)
						perror("read() failed");
					break;
				}
				
				msg_queue_end_enqueue(&server->clients[i].in, buffer, bytes_read);
			}
		}
		
		if (poll_fds[i+1].revents & POLLOUT){
			// We can send messages to the client. Loop either until we got no more messages
			// to send or we would block again.
			void* buffer = NULL;
			size_t buffer_size = 0;
			while( (buffer = msg_queue_start_dequeue(&server->clients[i].out, &buffer_size)) != NULL ){
				ssize_t bytes_written = write(poll_fds[i+1].fd, buffer, buffer_size);
				if (bytes_written != buffer_size)
					fprintf(stderr, "write() failed: only %zd bytes of %zu bytes written\n", bytes_written, buffer_size);
				if (bytes_written == -1)
					perror("write() failed");
				msg_queue_end_dequeue(&server->clients[i].out, buffer);
			}
		}
	}
}


void on_client_connect(ipc_server_p server, int client_fd){
	server->client_count++;
	server->clients = realloc(server->clients, server->client_count * sizeof(ipc_client_t));
	server->clients[server->client_count - 1] = (ipc_client_t){
		.fd = client_fd,
		.in = msg_queue_new(server->queue_length),
		.out = msg_queue_new(server->queue_length),
		.private = NULL
	};
	
	// Make client socket non blocking
	fcntl(client_fd, F_SETFL, fcntl(client_fd, F_GETFL) | O_NONBLOCK);
	
	// Mark this client as newly connected
	server->client_connected_idx = server->client_count - 1;
}

void on_client_disconnect(ipc_server_p server, size_t client_idx){
	// Mark this client as disconnected
	server->client_disconnected_private = server->clients[client_idx].private;
	
	// Free client resources
	close(server->clients[client_idx].fd);
	msg_queue_destroy(&server->clients[client_idx].in);
	msg_queue_destroy(&server->clients[client_idx].out);
	
	// Clean up the list
	for(size_t i = client_idx; i < server->client_count - 1; i++)
		memcpy(server->clients + i, server->clients + i + 1, sizeof(ipc_client_t));
	server->client_count--;
	server->clients = realloc(server->clients, server->client_count * sizeof(ipc_client_t));
}