#include <stdio.h>
#include "../msg.h"

int main(){
	uint8_t msg_buffer[PLAINS_MAX_MSG_SIZE];
	int fd_buffer[PLAINS_MAX_FD_COUNT];
	size_t msg_buffer_size, fd_buffer_size;
	plains_msg_t msg_in, msg_out;
	
	void test(plains_msg_p msg){
		msg_buffer_size = PLAINS_MAX_MSG_SIZE;
		fd_buffer_size = PLAINS_MAX_FD_COUNT;
		plains_msg_serialize(&msg_in, msg_buffer, &msg_buffer_size, fd_buffer, &fd_buffer_size);
		plains_msg_deserialize(&msg_out, msg_buffer, msg_buffer_size, fd_buffer, fd_buffer_size);
		
		printf("message buffer. %zu, fd buffer: %zu\n", msg_buffer_size, fd_buffer_size);
		printf("in:  ");
		plains_msg_print(&msg_in);
		printf("out: ");
		plains_msg_print(&msg_out);
	}
	
	test( msg_hello(&msg_in, 1, "test client", (uint16_t[]){0, 8, 15}, 3) );
	test( msg_draw(&msg_in, 1, NULL, 7, 100, 100, 10, 500, 500, -2, 0.5) );
}