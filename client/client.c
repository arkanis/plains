#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>

#include "client.h"
#include "net.h"

plains_con_p plains_connect(const char* socket_path){
	plains_con_p con = malloc(sizeof(plains_con_t));
	con->seq = 0;
	
	con->socket_fd = socket(AF_UNIX, SOCK_SEQPACKET, 0);
	if (con->socket_fd == -1){
		perror("socket() failed");
		free(con);
		return NULL;
	}
	
	struct sockaddr_un addr;
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

int plains_send(plains_con_p con, plains_msg_p msg){
	msg->seq = con->seq;
	
	int err = plains_msg_send(con->socket_fd, msg, con->send_msg_buffer, PLAINS_MAX_MSG_SIZE, con->send_fd_buffer, PLAINS_MAX_FD_COUNT);
	switch(err){
		case PLAINS_ESERIALIZE:
			fprintf(stderr, "plains_send(): plains_msg_serialize() failed");
			return -1;
		case PLAINS_EWRITE:
			perror("plains_send(): write() failed");
			return -1;
		case PLAINS_ESENDMSG:
			perror("plains_send(): sendmsg() failed");
			return -1;
		case PLAINS_EINCOMPLETE:
			// Error already printed by plains_msg_send()
			return -1;
	}
	
	con->seq++;
	return err;
}

int plains_receive(plains_con_p con, plains_msg_p msg){
	int err = plains_msg_receive(con->socket_fd, msg, con->receive_msg_buffer, PLAINS_MAX_MSG_SIZE, con->receive_fd_buffer, PLAINS_MAX_FD_COUNT);
	switch(err){
		case PLAINS_ERECVMSG:
			perror("plains_receive(): recvmsg() failed");
			return -1;
		case PLAINS_EDESERIALIZE:
			fprintf(stderr, "plains_receive(): plains_msg_deserialize() failed");
			return -1;
	}
	
	return err;
}