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
		
		void send(msg_p msg, int fd){
			ssize_t msg_size = msg_serialize(msg, buffer, MSG_MAX_SIZE);
			printf("%zd bytes serialized\n", msg_size);
			
			ssize_t bytes_written;
			if (fd == -1) {
				bytes_written = write(cfd, buffer, msg_size);
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
				
				struct cmsghdr *cmsg = CMSG_FIRSTHDR(&msghdr);
				cmsg->cmsg_level = SOL_SOCKET;
				cmsg->cmsg_type = SCM_RIGHTS;
				cmsg->cmsg_len = CMSG_LEN(sizeof(fd));
				
				int *fdptr = (int*) CMSG_DATA(cmsg);
				*fdptr = fd;
				msghdr.msg_controllen = cmsg->cmsg_len;
				
				bytes_written = sendmsg(cfd, &msghdr, 0);
				if (bytes_written == -1)
					perror("sendmsg() failed");
			}
			
			if (bytes_written != msg_size)
				printf("only %zu of %zu message bytes written!\n", bytes_written, msg_size);
		}
		
		ssize_t receive(msg_p msg){
			uint8_t aux_buffer[CMSG_SPACE(sizeof(int))];
			struct msghdr msghdr = (struct msghdr){
				.msg_name = NULL, .msg_namelen = 0,
				.msg_iov = (struct iovec[]){ {buffer, MSG_MAX_SIZE} },
				.msg_iovlen = 1,
				.msg_control = aux_buffer,
				.msg_controllen = sizeof(aux_buffer),
				.msg_flags = 0
			};
			ssize_t bytes_read = recvmsg(cfd, &msghdr, MSG_CMSG_CLOEXEC);
			if (bytes_read == 0)
				return 0;
			if (bytes_read == -1){
				perror("recvmsg() failed");
				return -1;
			}
			
			msg->fd = -1;
			for(struct cmsghdr *cm = CMSG_FIRSTHDR(&msghdr); cm != NULL; cm = CMSG_NXTHDR(&msghdr, cm)){
				printf("control message\n");
				if (cm->cmsg_level == SOL_SOCKET && cm->cmsg_type == SCM_RIGHTS){
					msg->fd = *( (int*) CMSG_DATA(cm) );
					break;
				}
			}
			
			ssize_t msg_size = msg_deserialize(msg, buffer, bytes_read);
			assert(bytes_read == msg_size);
			return msg_size;
		}
		
		send( msg_welcome(&msg, 1, "test server", NULL, 0), -1 );
		
		while( receive(&msg) > 0 ){
			char test[1024] = {0};
			ssize_t res = read(msg.fd, test, 1024);
			if (res == -1)
				perror("read() failed");
			close(msg.fd);
			printf("from fd %d: %s\n", msg.fd, test);
			
			msg_print(&msg);
			if (msg.type == MSG_LAYER_CREATE)
				send( msg_status(&msg, msg.seq, 0, 41), -1 );
		}
		
		printf("connection closed\n");
		if (close(cfd) == -1){
			perror("close");
			return -1;
		}
	}
	
	return 0;
}