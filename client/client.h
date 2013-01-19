#pragma once

#include "msg.h"


typedef struct {
	int socket_fd;
	uint16_t seq;
	
	uint8_t send_msg_buffer[PLAINS_MAX_MSG_SIZE];
	int send_fd_buffer[PLAINS_MAX_FD_COUNT];
	uint8_t receive_msg_buffer[PLAINS_MAX_MSG_SIZE];
	int receive_fd_buffer[PLAINS_MAX_FD_COUNT];
} plains_con_t, *plains_con_p;

plains_con_p plains_connect(const char* socket_path/*, const char* app_name*/);
void plains_disconnect(plains_con_p con);
int plains_send(plains_con_p con, plains_msg_p msg);
int plains_receive(plains_con_p con, plains_msg_p msg);