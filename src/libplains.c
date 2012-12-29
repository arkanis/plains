#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>

#include "libplains.h"

plains_con_p plains_connect(const char* socket_path){
	plains_con_p con = malloc(sizeof(plains_con_t));
	struct sockaddr_un addr;
	
	con->socket_fd = socket(AF_UNIX, SOCK_SEQPACKET, 0);
	if (con->socket_fd == -1){
		perror("socket() failed");
		free(con);
		return NULL;
	}
	
	memset(&addr, 0, sizeof(addr));
	addr.sun_family = AF_UNIX;
	strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path) - 1);
	
	if ( connect(con->socket_fd, (struct sockaddr *) &addr, sizeof(addr)) == -1 ){
		perror("connect() failed");
		free(con);
		return NULL;
	}
	
	return con;
}

void plains_disconnect(plains_con_p con){
	close(con->socket_fd);
	free(con);
}

int plains_send(plains_con_p con, msg_p msg){
	ssize_t msg_size = msg_serialize(msg, con->send_buffer, MSG_MAX_SIZE);
	ssize_t bytes_written = write(con->socket_fd, con->send_buffer, msg_size);
	if (bytes_written == -1) {
		perror("write() failed");
		return -1;
	} else if (bytes_written != msg_size) {
		printf("only %zu of %zu message bytes written!\n", bytes_written, msg_size);
		return -1;
	}
	return 1;
}

int plains_receive(plains_con_p con, msg_p msg){
	ssize_t bytes_read = read(con->socket_fd, con->receive_buffer, MSG_MAX_SIZE);
	if (bytes_read == -1){
		perror("read() failed");
		return -1;
	}
	if (bytes_read == 0)
		return 0;
	ssize_t msg_size = msg_deserialize(msg, con->receive_buffer, bytes_read);
	if (bytes_read != msg_size)
		return -1;
	return 1;
}