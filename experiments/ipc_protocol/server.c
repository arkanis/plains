#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <assert.h>

#include "msg.h"


int main(int argc, char *argv[]) {
	const char *server_file = "server.socket";
	const int backlog = 3;
	//const size_t buf_size = 4096;
	
	struct sockaddr_un addr;
	int sfd, cfd;
	//ssize_t numRead;
	//char buf[buf_size];

	sfd = socket(AF_UNIX, SOCK_SEQPACKET, 0);
	if (sfd == -1){
		perror("socket");
		return -1;
	}
	
	/* Construct server socket address, bind socket to it,
	   and make this a listening socket */
	if (unlink(server_file) == -1 && errno != ENOENT){
		perror("unlink");
		return -1;
	}
	
	
	memset(&addr, 0, sizeof(struct sockaddr_un));
	addr.sun_family = AF_UNIX;
	strncpy(addr.sun_path, server_file, sizeof(addr.sun_path) - 1);

	if (bind(sfd, (struct sockaddr *) &addr, sizeof(struct sockaddr_un)) == -1){
		perror("bind");
		return -1;
	}

	if (listen(sfd, backlog) == -1){
		perror("listen");
		return -1;
	}

	for (;;) {          /* Handle client connections iteratively */
		
		/* Accept a connection. The connection is returned on a new
		   socket, 'cfd'; the listening socket ('sfd') remains open
		   and can be used to accept further connections. */
		
		printf("waiting for connections...\n");
		cfd = accept(sfd, NULL, NULL);
		if (cfd == -1){
			perror("accept");
			return -1;
		}
		
		
		
		uint8_t buffer[MSG_MAX_SIZE];
		msg_t msg;
		
		void send(msg_p msg){
			ssize_t msg_size = msg_serialize(msg, buffer, MSG_MAX_SIZE);
			printf("%zd bytes serialized\n", msg_size);
			ssize_t bytes_written = write(cfd, buffer, msg_size);
			if (bytes_written == -1)
				perror("write");
			if (bytes_written != msg_size)
				printf("only %zu of %zu message bytes written!\n", bytes_written, msg_size);
		}
		
		ssize_t receive(msg_p msg){
			ssize_t bytes_read = read(cfd, buffer, MSG_MAX_SIZE);
			if (bytes_read == 0)
				return 0;
			if (bytes_read == -1){
				perror("read");
				return -1;
			}
			ssize_t msg_size = msg_deserialize(msg, buffer, bytes_read);
			assert(bytes_read == msg_size);
			return msg_size;
		}
		
		send( msg_welcome(&msg, 1, "test server", NULL, 0) );
		
		while( receive(&msg) > 0 ){
			msg_print(&msg);
			if (msg.type == MSG_LAYER_CREATE)
				send( msg_status(&msg, msg.seq, 0, 41) );
		}
		
		printf("connection closed\n");
		if (close(cfd) == -1){
			perror("close");
			return -1;
		}
	}
	
	return 0;
}