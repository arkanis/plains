#pragma once

#include "msg.h"


typedef struct {
	int socket_fd;
	uint8_t send_buffer[MSG_MAX_SIZE];
	uint8_t receive_buffer[MSG_MAX_SIZE];
} plains_con_t, *plains_con_p;

plains_con_p plains_connect(const char* socket_path/*, const char* app_name*/);
void plains_disconnect(plains_con_p con);
int plains_send(plains_con_p con, msg_p msg);
int plains_receive(plains_con_p con, msg_p msg);