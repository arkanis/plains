#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <assert.h>

#include <plains/net.h>


int main(int argc, char *argv[]) {
	const char *server_file = "server.socket";
	const int backlog = 3;
	
	struct sockaddr_un addr;
	int sfd, cfd;
	
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

	for (;;) {
		printf("waiting for connections...\n");
		cfd = accept(sfd, NULL, NULL);
		if (cfd == -1){
			perror("accept");
			return -1;
		}
		
		uint8_t msg_buffer[PLAINS_MAX_MSG_SIZE];
		int fd_buffer[PLAINS_MAX_FD_COUNT];
		plains_msg_t msg;
		
		void send(plains_msg_p message){
			int err = plains_msg_send(cfd, message, msg_buffer, PLAINS_MAX_MSG_SIZE, fd_buffer, PLAINS_MAX_FD_COUNT);
			switch(err){
				case PLAINS_ESERIALIZE:
					fprintf(stderr, "plains_send(): plains_msg_serialize() failed");
					break;
				case PLAINS_EWRITE:
					perror("plains_send(): write() failed");
					break;
				case PLAINS_ESENDMSG:
					perror("plains_send(): sendmsg() failed");
					break;
				case PLAINS_EINCOMPLETE:
					// Error already printed by plains_msg_send()
					break;
			}
		}
		
		int receive(plains_msg_p message){
			int err = plains_msg_receive(cfd, message, msg_buffer, PLAINS_MAX_MSG_SIZE, fd_buffer, PLAINS_MAX_FD_COUNT);
			switch(err){
				case PLAINS_ERECVMSG:
					perror("plains_receive(): recvmsg() failed");
					break;
				case PLAINS_EDESERIALIZE:
					fprintf(stderr, "plains_receive(): plains_msg_deserialize() failed");
					break;
			}
			return err;
		}
		
		
		send( msg_welcome(&msg, 1, "test server", NULL, 0) );
		
		while( receive(&msg) > 0 ){
			plains_msg_print(&msg);
			
			if (msg.type == PLAINS_MSG_DRAW){
				send( msg_status(&msg, msg.seq, 0, 41) );
				
				char test[1024] = {0};
				ssize_t res = read(msg.draw.shm_fd, test, 1024);
				if (res == -1)
					perror("read() failed");
				close(msg.draw.shm_fd);
				printf("from fd %d: %s\n", msg.draw.shm_fd, test);
				
				break;
			}
		}
		
		printf("connection closed\n");
		if (close(cfd) == -1){
			perror("close");
			return -1;
		}
	}
	
	return 0;
}