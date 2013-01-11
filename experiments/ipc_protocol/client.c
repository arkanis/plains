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
	
	void send(msg_p msg, int fd){
		ssize_t msg_size = msg_serialize(msg, buffer, MSG_MAX_SIZE);
		ssize_t bytes_written;
		if (fd == -1) {
			bytes_written = write(sfd, buffer, msg_size);
			if (bytes_written == -1)
				perror("write() failed");
		} else {
			uint8_t aux_buffer[CMSG_SPACE(sizeof(fd))];
			struct msghdr msghdr = (struct msghdr){
				.msg_name = NULL, .msg_namelen = 0,
				.msg_iov = (struct iovec[]){ {buffer, msg_size} },
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
			
			bytes_written = sendmsg(sfd, &msghdr, 0);
			if (bytes_written == -1)
				perror("sendmsg() failed");
		}
		
		if (bytes_written != msg_size)
			printf("only %zu of %zu message bytes written!\n", bytes_written, msg_size);
	}
	
	void receive(msg_p msg){
		uint8_t aux_buffer[CMSG_SPACE(sizeof(int))];
		struct msghdr msghdr = (struct msghdr){
			.msg_name = NULL, .msg_namelen = 0,
			.msg_iov = (struct iovec[]){ {buffer, MSG_MAX_SIZE} },
			.msg_iovlen = 1,
			.msg_control = aux_buffer,
			.msg_controllen = sizeof(aux_buffer),
			.msg_flags = 0
		};
		ssize_t bytes_read = recvmsg(sfd, &msghdr, MSG_CMSG_CLOEXEC);
		
		msg->fd = -1;
		for(struct cmsghdr *cm = CMSG_FIRSTHDR(&msghdr); cm != NULL; cm = CMSG_NXTHDR(&msghdr, cm)){
			if (cm->cmsg_level == SOL_SOCKET && cm->cmsg_type == SCM_RIGHTS){
				msg->fd = *( (int*) CMSG_DATA(cm) );
				break;
			}
		}
		
		if (bytes_read == 0)
			return;
		if (bytes_read == -1){
			perror("read");
			return;
		}
		ssize_t msg_size = msg_deserialize(msg, buffer, bytes_read);
		assert(bytes_read == msg_size);
	}
	
	int fds[2];
	
	ssize_t res = pipe(fds);
	if (res == -1)
		perror("pipe() failed");
	send( msg_hello(&msg, 1, "test client", (uint16_t[]){ 1, 7, 42 }, 3), fds[0] );
	res = write(fds[1], "hello\0", 6);
	if (res == -1)
		perror("write() failed");
	close(fds[0]);
	close(fds[1]);
	
	receive(&msg);
	msg_print(&msg);
	
	pipe(fds);
	send( msg_layer_create(&msg, 0, 0, 0, 800, 600), fds[0] );
	res = write(fds[1], "hello\0", 6);
	if (res == -1)
		perror("write() failed");
	close(fds[0]);
	close(fds[1]);
	
	receive(&msg);
	msg_print(&msg);
	
	close(sfd);
	
	return 0;
}