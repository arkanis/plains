#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <stdio.h>

#include "net.h"

int plains_msg_send(int socket_fd, plains_msg_p msg, void* msg_buffer, size_t msg_buffer_size, int* fd_buffer, size_t fd_buffer_length){
	if ( plains_msg_serialize(msg, msg_buffer, &msg_buffer_size, fd_buffer, &fd_buffer_length) < 0 )
		return PLAINS_ESERIALIZE;
	
	ssize_t bytes_send = 0;
	if (fd_buffer_length == 0) {
		// No fd to send, we can just use write
		bytes_send = write(socket_fd, msg_buffer, msg_buffer_size);
		if (bytes_send == -1)
			return PLAINS_EWRITE;
	} else {
		uint8_t aux_buffer[CMSG_SPACE(fd_buffer_length * sizeof(int))];
		struct msghdr msghdr = (struct msghdr){
			.msg_name = NULL, .msg_namelen = 0,
			.msg_iov = (struct iovec[]){ {msg_buffer, msg_buffer_size} },
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
			cm_fp[i] = fd_buffer[i];
		msghdr.msg_controllen = cm->cmsg_len;
		
		bytes_send = sendmsg(socket_fd, &msghdr, 0);
		if (bytes_send == -1)
			return PLAINS_ESENDMSG;
	}
	
	if (bytes_send != msg_buffer_size) {
		printf("plains_send(): only %zu of %zu message bytes send\n", bytes_send, msg_buffer_size);
		return PLAINS_EINCOMPLETE;
	}
	
	return 1;
}

int plains_msg_receive(int socket_fd, plains_msg_p msg, void* msg_buffer, size_t msg_buffer_size, int* fd_buffer, size_t fd_buffer_length){
	uint8_t aux_buffer[CMSG_SPACE(fd_buffer_length * sizeof(int))];
	struct msghdr msghdr = (struct msghdr){
		.msg_name = NULL, .msg_namelen = 0,
		.msg_iov = (struct iovec[]){ {msg_buffer, msg_buffer_size} },
		.msg_iovlen = 1,
		.msg_control = aux_buffer,
		.msg_controllen = sizeof(aux_buffer),
		.msg_flags = 0
	};
	ssize_t bytes_received = recvmsg(socket_fd, &msghdr, MSG_CMSG_CLOEXEC);
	
	// Connection shutdown (EOF)
	if (bytes_received == 0)
		return 0;
	
	if (bytes_received == -1)
		return PLAINS_ERECVMSG;
	
	size_t received_fd_count = 0;
	for(struct cmsghdr *cm = CMSG_FIRSTHDR(&msghdr); cm != NULL; cm = CMSG_NXTHDR(&msghdr, cm)){
		if (cm->cmsg_level == SOL_SOCKET && cm->cmsg_type == SCM_RIGHTS){
			int* cm_fp = (int*) CMSG_DATA(cm);
			received_fd_count = (cm->cmsg_len - ((void*)cm_fp - (void*)cm)) / sizeof(int);
			for(size_t i = 0; i < received_fd_count; i++)
				fd_buffer[i] = cm_fp[i];
			break;
		}
	}
	
	if ( plains_msg_deserialize(msg, msg_buffer, bytes_received, fd_buffer, received_fd_count) < 0 )
		return PLAINS_EDESERIALIZE;
	
	return 1;
}