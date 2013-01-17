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

int plains_send(plains_con_p con, plains_msg_p msg){
	msg->seq = con->seq;
	
	size_t msg_buffer_size = PLAINS_MAX_MSG_SIZE, fd_buffer_length = PLAINS_MAX_FD_COUNT;
	if ( plains_msg_serialize(msg, con->send_msg_buffer, &msg_buffer_size, con->send_fd_buffer, &fd_buffer_length) < 0 ) {
		fprintf(stderr, "plains_send(): plains_msg_serialize() failed");
		return -1;
	}
	
	ssize_t bytes_send = 0;
	if (fd_buffer_length == 0) {
		// No fd to send, we can just use write
		bytes_send = write(con->socket_fd, con->send_msg_buffer, msg_buffer_size);
		if (bytes_send == -1) {
			perror("plains_send(): write() failed");
			return -1;
		}
	} else {
		uint8_t aux_buffer[CMSG_SPACE(fd_buffer_length * sizeof(int))];
		struct msghdr msghdr = (struct msghdr){
			.msg_name = NULL, .msg_namelen = 0,
			.msg_iov = (struct iovec[]){ {con->send_msg_buffer, msg_buffer_size} },
			.msg_iovlen = 1,
			.msg_control = aux_buffer,
			.msg_controllen = sizeof(aux_buffer),
			.msg_flags = 0
		};
		
		struct cmsghdr *cm = CMSG_FIRSTHDR(&msghdr);
		cm->cmsg_level = SOL_SOCKET;
		cm->cmsg_type = SCM_RIGHTS;
		cm->cmsg_len = CMSG_LEN(fd_buffer_length * sizeof(int));
		
		int* cm_fp = (int*) CMSG_DATA(cm);
		for(size_t i = 0; i < fd_buffer_length; i++)
			cm_fp[i] = con->send_fd_buffer[i];
		msghdr.msg_controllen = cm->cmsg_len;
		
		bytes_send = sendmsg(con->socket_fd, &msghdr, 0);
		if (bytes_send == -1) {
			perror("plains_send(): sendmsg() failed");
			return -1;
		}
	}
	
	if (bytes_send != msg_buffer_size) {
		printf("plains_send(): only %zu of %zu message bytes send\n", bytes_send, msg_buffer_size);
		return -1;
	}
	
	con->seq++;
	return 0;
}

int plains_receive(plains_con_p con, plains_msg_p msg){
	uint8_t aux_buffer[CMSG_SPACE(sizeof(con->receive_fd_buffer))];
	struct msghdr msghdr = (struct msghdr){
		.msg_name = NULL, .msg_namelen = 0,
		.msg_iov = (struct iovec[]){ {con->receive_msg_buffer, sizeof(con->receive_msg_buffer)} },
		.msg_iovlen = 1,
		.msg_control = aux_buffer,
		.msg_controllen = sizeof(aux_buffer),
		.msg_flags = 0
	};
	ssize_t bytes_received = recvmsg(con->socket_fd, &msghdr, MSG_CMSG_CLOEXEC);
	
	if (bytes_received == 0) {
		// Connection shutdown (EOF)
		return 0;
	}
		
	if (bytes_received == -1) {
		perror("plains_receive(): recvmsg() failed");
		return -1;
	}
	
	size_t received_fd_count = 0;
	for(struct cmsghdr *cm = CMSG_FIRSTHDR(&msghdr); cm != NULL; cm = CMSG_NXTHDR(&msghdr, cm)){
		if (cm->cmsg_level == SOL_SOCKET && cm->cmsg_type == SCM_RIGHTS){
			int* cm_fp = (int*) CMSG_DATA(cm);
			received_fd_count = (cm->cmsg_len - ((void*)cm_fp - (void*)cm)) / sizeof(int);
			for(size_t i = 0; i < received_fd_count; i++)
				con->receive_fd_buffer[i] = cm_fp[i];
			break;
		}
	}
	
	if ( plains_msg_deserialize(msg, con->receive_msg_buffer, bytes_received, con->receive_fd_buffer, received_fd_count) < 0 ) {
		fprintf(stderr, "plains_receive(): plains_msg_deserialize() failed");
		return -1;
	}
	
	return 1;
}