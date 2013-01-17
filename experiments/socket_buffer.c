#include <sys/socket.h>
#include <sys/un.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <stdio.h>

void main(){
	int fds[2];
	int err = socketpair(AF_UNIX, SOCK_SEQPACKET, 0, fds);
	if (err == -1)
		perror("socketpair() failed");
	
	int result, val;
	socklen_t len = sizeof(val);
	result = getsockopt(fds[0], SOL_SOCKET, SO_SNDBUF, &val, &len);
	if (err == -1)
		perror("getsockopt() failed");
	printf("SO_SNDBUF %d: %d bytes, max %d messages\n", len, val, val / 213);
	
	val = 213 * 100;
	result = setsockopt(fds[0], SOL_SOCKET, SO_SNDBUF, &val, len);
	if (err == -1)
		perror("setsockopt() failed");
	
	result = getsockopt(fds[0], SOL_SOCKET, SO_SNDBUF, &val, &len);
	if (err == -1)
		perror("getsockopt() failed");
	printf("SO_SNDBUF %d: %d bytes, max %d messages\n", len, val, val / 213);
	
	result = ioctl(fds[0], FIONREAD, &val);
	if (err == -1)
		perror("ioctl() failed");
	printf("FIONREAD: %d bytes\n", val);
	
	char buf[] = "hello";
	write(fds[1], buf, sizeof(buf));
	
	result = ioctl(fds[0], FIONREAD, &val);
	if (err == -1)
		perror("ioctl() failed");
	printf("FIONREAD: %d bytes\n", val);
}