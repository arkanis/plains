#include <unistd.h>
#include <string.h>
#include "../libplains.h"

int main(int argc, char **argv){
	plains_msg_t msg;
	
	plains_con_p con = plains_connect("server.socket");
	plains_send(con, msg_hello(&msg, 1, "test client", NULL, 0));
	
	int pipe_fds[2];
	pipe(pipe_fds);
	write(pipe_fds[1], argv[1], strlen(argv[1]));
	plains_send(con, msg_draw(&msg, 0, NULL, pipe_fds[0], 0, 0, 0, 10, 10, 0, 1.0));
	
	while ( plains_receive(con, &msg) > 0 ) {
		plains_msg_print(&msg);
	}
	
	plains_disconnect(con);
}