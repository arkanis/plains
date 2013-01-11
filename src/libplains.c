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

int plains_send(plains_con_p con, msg_p msg){
	msg->seq = con->seq;
	ssize_t msg_size = msg_serialize(msg, con->send_buffer, MSG_MAX_SIZE);
	ssize_t bytes_written = write(con->socket_fd, con->send_buffer, msg_size);
	if (bytes_written == -1) {
		perror("write() failed");
		return -1;
	} else if (bytes_written != msg_size) {
		printf("only %zu of %zu message bytes written!\n", bytes_written, msg_size);
		return -1;
	}
	con->seq++;
	return 1;
}

int plains_send_with_fd(plains_con_p con, msg_p msg, int fd){
	msg->seq = con->seq;
	ssize_t msg_size = msg_serialize(msg, con->send_buffer, sizeof(con->send_buffer));
	
	uint8_t aux_buffer[CMSG_SPACE(sizeof(fd))];
	struct msghdr msghdr = (struct msghdr){
		.msg_name = NULL, .msg_namelen = 0,
		.msg_iov = (struct iovec[]){ {con->send_buffer, msg_size} },
		.msg_iovlen = 1,
		.msg_control = aux_buffer,
		.msg_controllen = sizeof(aux_buffer),
		.msg_flags = 0
	};
	
	struct cmsghdr *cm = CMSG_FIRSTHDR(&msghdr);
	cm->cmsg_level = SOL_SOCKET;
	cm->cmsg_type = SCM_RIGHTS;
	cm->cmsg_len = CMSG_LEN(sizeof(fd));
	
	int *fdptr = (int*) CMSG_DATA(cm);
	*fdptr = fd;
	msghdr.msg_controllen = cm->cmsg_len;
	
	ssize_t bytes_send = sendmsg(con->socket_fd, &msghdr, 0);
	if (bytes_send == -1) {
		perror("sendmsg() failed");
		return -1;
	} else if (bytes_send != msg_size) {
		printf("only %zu of %zu message bytes send!\n", bytes_send, msg_size);
		return -1;
	}
	
	con->seq++;
	return 1;
}

int plains_receive(plains_con_p con, msg_p msg){
	uint8_t aux_buffer[CMSG_SPACE(sizeof(int))];
	struct msghdr msghdr = (struct msghdr){
		.msg_name = NULL, .msg_namelen = 0,
		.msg_iov = (struct iovec[]){ {con->receive_buffer, sizeof(con->receive_buffer)} },
		.msg_iovlen = 1,
		.msg_control = aux_buffer,
		.msg_controllen = sizeof(aux_buffer),
		.msg_flags = 0
	};
	ssize_t bytes_received = recvmsg(con->socket_fd, &msghdr, MSG_CMSG_CLOEXEC);
	
	if (bytes_received == 0)
		return 0;
	if (bytes_received == -1){
		perror("recvmsg");
		return -1;
	}
	
	ssize_t msg_size = msg_deserialize(msg, con->receive_buffer, bytes_received);
	if (bytes_received != msg_size)
		return -1;
	
	msg->fd = -1;
	for(struct cmsghdr *cm = CMSG_FIRSTHDR(&msghdr); cm != NULL; cm = CMSG_NXTHDR(&msghdr, cm)){
		if (cm->cmsg_level == SOL_SOCKET && cm->cmsg_type == SCM_RIGHTS){
			msg->fd = *( (int*) CMSG_DATA(cm) );
			break;
		}
	}
	
	return 1;
}