#include <stdbool.h>
#include <stdio.h>

#include "ipc_server.h"

int main(int argc, char **argv){
	ipc_server_p server = ipc_server_new("server.socket", 3, 20);
	msg_t msg;
	
	while(true){
		ipc_server_cycle(server, 1000);
		
		if (server->client_connected_idx != -1){
			printf("new client: %zu\n", server->client_connected_idx);
			server->clients[server->client_connected_idx].private = &server->clients[server->client_connected_idx];
			
			ipc_server_send(server, server->client_connected_idx, msg_welcome(&msg, 1, "test server", NULL, 0) );
		}
		
		if (server->client_disconnected_private != NULL){
			printf("client disconnected: %p\n", server->client_disconnected_private);
		}
		
		for(size_t i = 0; i < server->client_count; i++){
			ipc_client_p client = &server->clients[i];
			
			size_t buffer_size = 0;
			void *buffer = NULL;
			while( (buffer = msg_queue_start_dequeue(&client->in, &buffer_size)) != NULL ){
				printf("client %zu: ", i);
				
				msg_deserialize(&msg, buffer, buffer_size);
				msg_print(&msg);
				switch(msg.type){
					case MSG_LAYER_CREATE:
						ipc_server_send(server, i, msg_status(&msg, msg.seq, 0, 42) );
						break;
				}
				
				msg_queue_end_dequeue(&client->in, buffer);
			}
		}
	}
	
	ipc_server_destroy(server);
}