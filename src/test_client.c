#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>

#include "libplains.h"


int main(int argc, char *argv[]) {
	plains_con_p con = plains_connect("server.socket");
	msg_t msg;
	
	plains_send(con, msg_hello(&msg, 1, "test client", (uint16_t[]){ 1, 7, 42 }, 3) );
	plains_receive(con, &msg);
	msg_print(&msg);
	
	plains_send(con, msg_layer_create(&msg, 0, 0, 0, 800, 600) );
	plains_receive(con, &msg);
	msg_print(&msg);
	
	plains_disconnect(con);
	return 0;
}