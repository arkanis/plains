#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include <string.h>
#include <stdio.h>
#include <assert.h>

#include "msg.h"


int main(int argc, char *argv[]) {
	const char *server_file = "server.socket";
	
	struct sockaddr_un addr;
	int sfd;
	
	sfd = socket(AF_UNIX, SOCK_SEQPACKET, 0);
	if (sfd == -1){
		perror("socket");
		return -1;
	}
	
	memset(&addr, 0, sizeof(struct sockaddr_un));
	addr.sun_family = AF_UNIX;
	strncpy(addr.sun_path, server_file, sizeof(addr.sun_path) - 1);
	
	if (connect(sfd, (struct sockaddr *) &addr, sizeof(struct sockaddr_un)) == -1){
		perror("connect");
		return -1;
	}
	
	uint8_t buffer[MSG_MAX_SIZE];
	msg_t msg;
	
	void send(msg_p msg){
		ssize_t msg_size = msg_serialize(msg, buffer, MSG_MAX_SIZE);
		ssize_t bytes_written = write(sfd, buffer, msg_size);
		if (bytes_written != msg_size)
			printf("only %zu of %zu message bytes written!\n", bytes_written, msg_size);
	}
	
	void receive(msg_p msg){
		ssize_t bytes_read = read(sfd, buffer, MSG_MAX_SIZE);
		if (bytes_read == 0)
			return;
		if (bytes_read == -1){
			perror("read");
			return;
		}
		ssize_t msg_size = msg_deserialize(msg, buffer, bytes_read);
		assert(bytes_read == msg_size);
	}
	
	
	send( msg_hello(&msg, 1, "test client", (uint16_t[]){ 1, 7, 42 }, 3) );
	receive(&msg);
	msg_print(&msg);
	
	send( msg_layer_create(&msg, 0, 0, 0, 800, 600) );
	receive(&msg);
	msg_print(&msg);
	
	close(sfd);
	
	return 0;
}