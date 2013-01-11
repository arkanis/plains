#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <assert.h>

#include <stdio.h>
#include <stdlib.h>
#define __USE_XOPEN 1
#include <math.h>

#include <SDL/SDL.h>

#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>

#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "common.h"
#include "math.h"
#include "viewport.h"
#include "layer.h"
#include "tile_table.h"
#include "ipc_server.h"

#include "image_window.c"
#include "image_haruhi.c"



/*

Resource actions:
- load
- new
- destroy
- unload

Units:
- World space: meter [m]

Pipeline:
- transform * local coords -> world coords
- camera * world coords -> normalized device coords
- viewport * normalized device coords -> screen coords

*/

// Viewport of the renderer. Data from the viewport is used by other components.
viewport_p viewport;
tile_table_p tile_table;


//
// Grid
//
GLuint grid_prog, grid_vertex_buffer;
// Space between grid lines in world units
vec2_t grid_default_spacing = {100, 100};

void grid_load(){
	grid_prog = load_and_link_program("grid.vs", "grid.ps");
	assert(grid_prog != 0);
	
	glGenBuffers(1, &grid_vertex_buffer);
	assert(grid_vertex_buffer != 0);
	glBindBuffer(GL_ARRAY_BUFFER, grid_vertex_buffer);
	
	// Rectangle
	const float vertecies[] = {
		1, 1,
		-1, 1,
		-1, -1,
		1, -1
	};
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertecies), vertecies, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void grid_unload(){
	glDeleteBuffers(1, &grid_vertex_buffer);
	delete_program_and_shaders(grid_prog);
}

void grid_draw(){
	vec2_t grid_spacing = (vec2_t){
		grid_default_spacing.x * viewport->world_to_screen[0],
		grid_default_spacing.y * viewport->world_to_screen[4]
	};
	vec2_t grid_offset = (vec2_t){
		viewport->pos.x * viewport->world_to_screen[0],
		viewport->pos.y * viewport->world_to_screen[4]
	};
	//printf("grid_spacing: %f %f, grid_offset: %f %f\n", grid_spacing.x, grid_spacing.y, grid_offset.x, grid_offset.y);
	
	glUseProgram(grid_prog);
	glBindBuffer(GL_ARRAY_BUFFER, grid_vertex_buffer);
	
	GLint pos_attrib = glGetAttribLocation(grid_prog, "pos");
	glEnableVertexAttribArray(pos_attrib);
	glVertexAttribPointer(pos_attrib, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, 0);
	
	glUniform4f( glGetUniformLocation(grid_prog, "color"), 0.5, 0.5, 0.5, 1 );
	glUniform2f( glGetUniformLocation(grid_prog, "grid_spacing"), grid_spacing.x, grid_spacing.y );
	glUniform2f( glGetUniformLocation(grid_prog, "grid_offset"), grid_offset.x, grid_offset.y );
	glUniform1f( glGetUniformLocation(grid_prog, "vp_scale_exp"), viewport->scale_exp );
	glUniform2f( glGetUniformLocation(grid_prog, "screen_size"), viewport->screen_size.x, viewport->screen_size.y );
	glDrawArrays(GL_QUADS, 0, 4);
	
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glUseProgram(0);
}


//
// Cursor
//
vec2_t cursor_pos = {0, 0};
GLuint cursor_prog, cursor_vertex_buffer;

void cursor_load(){
	cursor_prog = load_and_link_program("cursor.vs", "cursor.ps");
	assert(cursor_prog != 0);
	
	glGenBuffers(1, &cursor_vertex_buffer);
	assert(cursor_vertex_buffer != 0);
	glBindBuffer(GL_ARRAY_BUFFER, cursor_vertex_buffer);
	
	// Rectangle
	const float vertecies[] = {
		5, 5,
		-5, 5,
		-5, -5,
		5, -5
	};
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertecies), vertecies, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void cursor_unload(){
	glDeleteBuffers(1, &cursor_vertex_buffer);
	delete_program_and_shaders(cursor_prog);
}

void cursor_draw(){
	//printf("cursor pos: x %f y %f\n", cursor_pos.x, cursor_pos.y);
	
	glUseProgram(cursor_prog);
	glBindBuffer(GL_ARRAY_BUFFER, cursor_vertex_buffer);
	
	GLint pos_attrib = glGetAttribLocation(cursor_prog, "pos");
	glEnableVertexAttribArray(pos_attrib);
	glVertexAttribPointer(pos_attrib, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, 0);
	
	GLint cursor_pos_uni = glGetUniformLocation(cursor_prog, "cursor_pos");
	assert(cursor_pos_uni != -1);
	glUniform2f(cursor_pos_uni, cursor_pos.x, cursor_pos.y);
	
	GLint color_uni = glGetUniformLocation(cursor_prog, "color");
	assert(color_uni != -1);
	glUniform4f(color_uni, 0, 0, 0, 1);
	
	GLint projection_uni = glGetUniformLocation(cursor_prog, "projection");
	assert(projection_uni != -1);
	glUniformMatrix3fv(projection_uni , 1, GL_FALSE, viewport->screen_to_normal);
	
	glDrawArrays(GL_QUADS, 0, 4);
	
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glUseProgram(0);
}


//
// Debuging stuff
//
GLuint debug_vertex_buffer, debug_prog;

void debug_load(){
	debug_prog = load_and_link_program("tiles.vs", "tiles.ps");
	glGenBuffers(1, &debug_vertex_buffer);
	assert(debug_vertex_buffer != 0);
	/*
	size_t tile_count = tile_table_tile_count_for_size(tile_table, image_haruhi.width, image_haruhi.height);
	tile_id_t tile_ids[tile_count];
	
	tile_table_alloc(tile_table, tile_count, tile_ids, NULL);
	tile_table_upload(tile_table, tile_count, tile_ids, image_haruhi.width, image_haruhi.height, image_haruhi.pixel_data);
	*/
}

void debug_unload(){
	glDeleteBuffers(1, &debug_vertex_buffer);
	delete_program_and_shaders(debug_prog);
}

/**
 * Renders the tile table texture at the origin.
 */
void debug_draw(){
	uint32_t x = 1000, y = 1000;
	uint32_t w = tile_table->width, h = tile_table->height;
	glBindBuffer(GL_ARRAY_BUFFER, debug_vertex_buffer);
	const float vertecies[] = {
		x+0, y+0, 0, 0,
		x+w, y+0, w, 0,
		x+w, y+h, w, h,
		x+0, y+h, 0, h
	};
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertecies), vertecies, GL_STATIC_DRAW);
	
	glUseProgram(debug_prog);
	GLint pos_tex_attrib = glGetAttribLocation(debug_prog, "pos_and_tex");
	assert(pos_tex_attrib != -1);
	glEnableVertexAttribArray(pos_tex_attrib);
	glVertexAttribPointer(pos_tex_attrib, 4, GL_FLOAT, GL_FALSE, sizeof(float) * 4, 0);
	
	GLint world_to_norm_uni = glGetUniformLocation(debug_prog, "world_to_norm");
	assert(world_to_norm_uni != -1);
	glUniformMatrix3fv(world_to_norm_uni, 1, GL_FALSE, viewport->world_to_normal);
	
	glActiveTexture(GL_TEXTURE0);
	GLint tex_attrib = glGetUniformLocation(debug_prog, "tex");
	assert(tex_attrib != -1);
	glUniform1i(tex_attrib, 0);
	
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, tile_table->texture);
	glDrawArrays(GL_QUADS, 0, 4);
	
	glUseProgram(0);
	glBindTexture(GL_TEXTURE_2D, 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

//
// Renderer
//
void renderer_resize(uint16_t window_width, uint16_t window_height){
	SDL_SetVideoMode(window_width, window_height, 24, SDL_OPENGL | SDL_RESIZABLE);
	glViewport(0, 0, window_width, window_height);
	vp_screen_changed(viewport, window_width, window_height);
}

void renderer_load(uint16_t window_width, uint16_t window_height, const char *title){
	// Create window
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_WM_SetCaption(title, NULL);
	
	// Initialize viewport structure
	viewport = vp_new((vec2_t){window_width, window_height}, 2);
	renderer_resize(window_width, window_height);
	
	// Enable alpha blending
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	
	glEnable(GL_LINE_SMOOTH);
	glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
}

void renderer_unload(){
	vp_destroy(viewport);
}

void renderer_clear(){
	glClearColor(1, 1, 1, 1.0);
	glClear(GL_COLOR_BUFFER_BIT);
}

void renderer_draw_response(draw_request_p req){
	layer_p layer = &layers[req->layer_idx];
	layer_scale_p ls = layer->current_scale;
	
	// Create a new layer scale if neccessary
	if (ls == NULL || ls->scale_index != req->scale_exp) {
		layer_scale_new(layer, req->scale_exp, tile_table, viewport);
		ls = layer->current_scale;
	}
	
	// Upload pixel data if we have it
	if (req->flags & DRAW_REQUEST_ANSWERED) {
		// Map shared memory and upload the pixel data. When done unmap the
		// shared memory and close the shared memory file (it will be deleted now).
		size_t shm_size = req->w * req->h * 4;
		void *pixel_data = mmap(NULL, shm_size, PROT_READ, MAP_SHARED, fd, 0);
		//layer_scale_upload(layer, req->scale_exp, pixel_data, tile_table);
		// TODO: implement proper upload function
		layer_scale_upload(layer, req->scale_exp, req->x, req->y, req->w, req->h, pixel_data, tile_table);
		munmap(pixel_data, shm_size);
		close(fd);
	}
	
	// Allocate vertex buffer object for tiles
	size_t tile_count = tile_table_tile_count_for_size(tile_table, req->w, req->h);
	glBindBuffer(GL_ARRAY_BUFFER, layers_vertex_buffer);
	glBufferData(GL_ARRAY_BUFFER, tile_count * 16 * sizeof(float), NULL, GL_STREAM_DRAW);
	float *buffer = glMapBuffer(GL_ARRAY_BUFFER, GL_READ_WRITE);
	
	
	if (request->flags & DRAW_REQUEST_PLACEHOLDER) {
		// Use placeholder tile, shared memory already freed
	} else {
	}
	
	// bo is short for buffer_offset
	size_t bo = 0;
	size_t t = tile_table->tile_size;
	size_t tiles_to_draw = 0;
	
	for(size_t i = 0; i < layer_count; i++){
		layer_p layer = &layers[i];
		layer_scale_p ls = layers[i].current_scale;
		// Ignore all layers that do not yet have a scale
		if (ls == NULL)
			continue;
		float scale = vp_scale_for(viewport, ls->scale_index);
		
		// world space tile size (the smaller the scale the larger a tile becomes)
		size_t ws_t = t / scale;
		
		for(size_t j = 0; j < ls->tile_count; j++){
			uint64_t x, y;
			layer_scale_tile_id_to_offset(ls, j, tile_table, &x, &y);
			float ws_x = x / scale, ws_y = y / scale;
			
			uint32_t u, v;
			tile_table_id_to_offset(tile_table, ls->tile_ids[j], &u, &v);
			size_t w, h;
			w = (ls->width - x > t) ? t : ls->width - x;
			h = (ls->height - y > t) ? t : ls->height - y;
			float ws_w = w / scale, ws_h = h / scale;
			
			// Unfortunately we need all this casting stuff otherwise C will only
			// use unsigned values and break all negative coordinates.
			buffer[bo++] = layer->x + ws_x +    0;  buffer[bo++] = layer->y + ws_y +    0;  buffer[bo++] = u + (float)0;  buffer[bo++] = v + (float)0;
			buffer[bo++] = layer->x + ws_x + ws_w;  buffer[bo++] = layer->y + ws_y +    0;  buffer[bo++] = u + (float)w;  buffer[bo++] = v + (float)0;
			buffer[bo++] = layer->x + ws_x + ws_w;  buffer[bo++] = layer->y + ws_y + ws_h;  buffer[bo++] = u + (float)w;  buffer[bo++] = v + (float)h;
			buffer[bo++] = layer->x + ws_x +    0;  buffer[bo++] = layer->y + ws_y + ws_h;  buffer[bo++] = u + (float)0;  buffer[bo++] = v + (float)h;
			
			tiles_to_draw++;
			// Reset the tile age since we'll use it in this draw operation
			tile_table->tile_ages[ls->tile_ids[j]] = 0;
		}
	}
	/*
	printf("vertex buffer:\n");
	for(size_t i = 0; i < bo; i += 16){
		printf("  %f,%f (%.0f,%.0f)  %f,%f (%.0f,%.0f)\n  %f,%f (%.0f,%.0f)  %f,%f (%.0f,%.0f)\n--\n",
			buffer[i+0], buffer[i+1], buffer[i+2], buffer[i+3],
			buffer[i+4], buffer[i+5], buffer[i+6], buffer[i+7],
			buffer[i+8], buffer[i+9], buffer[i+10], buffer[i+11],
			buffer[i+12], buffer[i+13], buffer[i+14], buffer[i+15]
		);
	}
	*/
	
	glUnmapBuffer(GL_ARRAY_BUFFER);
	
	
	glUseProgram(layer_prog);
	
	GLint pos_tex_attrib = glGetAttribLocation(layer_prog, "pos_and_tex");
	assert(pos_tex_attrib != -1);
	glEnableVertexAttribArray(pos_tex_attrib);
	glVertexAttribPointer(pos_tex_attrib, 4, GL_FLOAT, GL_FALSE, sizeof(float) * 4, 0);
	
	GLint world_to_norm_uni = glGetUniformLocation(layer_prog, "world_to_norm");
	assert(world_to_norm_uni != -1);
	glUniformMatrix3fv(world_to_norm_uni, 1, GL_FALSE, viewport->world_to_normal);
	
	glActiveTexture(GL_TEXTURE0);
	GLint tex_attrib = glGetUniformLocation(layer_prog, "tex");
	assert(tex_attrib != -1);
	glUniform1i(tex_attrib, 0);
	
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, tile_table->texture);
	//glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	//glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glDrawArrays(GL_QUADS, 0, tiles_to_draw * 4);
	
	glBindTexture(GL_TEXTURE_2D, 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glUseProgram(0);
	
	tile_table_cycle(tile_table);
}

void render_finish_draw(){
	cursor_draw();
	debug_draw();
	
	SDL_GL_SwapBuffers();
}


int main(int argc, char **argv){
	uint16_t win_w = 640, win_h = 480;
	
	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER);
	renderer_load(win_w, win_h, "Grid");
	
	grid_load();
	cursor_load();
	layers_load();
	
	tile_table = tile_table_new(2048, 2048, 128);
	
	
	layer_new(0, 0, 0, image_haruhi.width * 2, image_haruhi.height * 2, NULL);
	layer_scale_new(&layers[0], -1, tile_table, viewport);
	layer_scale_upload(&layers[0], -1, image_haruhi.pixel_data, tile_table);
	
	layer_new(0, 500, 0, image_window.width, image_window.height, NULL);
	layer_scale_new(&layers[1], 0, tile_table, viewport);
	layer_scale_upload(&layers[1], 0, image_window.pixel_data, tile_table);
	
	layer_new(0, -500, 0, image_window.width, image_window.height, NULL);
	layer_scale_new(&layers[2], 0, tile_table, viewport);
	layer_scale_upload(&layers[2], 0, image_window.pixel_data, tile_table);
	/*
	layer_new(-700, -500, 0, image_haruhi.width, image_haruhi.height, NULL);
	layer_scale_new(&layers[3], 0, tile_table, viewport);
	layer_scale_upload(&layers[3], 0, image_haruhi.pixel_data, tile_table);
	*/
	
	debug_load();
	
	ipc_server_p server = ipc_server_new("server.socket", 3, 20);
	msg_t msg;
	draw_request_tree_p draw_requests, retained_draw_requests;
	
	SDL_Event e;
	bool quit = false, viewport_grabbed = false, trigger_redraw = false;
	
	while (!quit) {
		ipc_server_cycle(server, 10);
		
		if (server->client_connected_idx != -1){
			printf("new client: %zu\n", server->client_connected_idx);
			server->clients[server->client_connected_idx].private = &server->clients[server->client_connected_idx];
			
			ipc_server_send(server, server->client_connected_idx, msg_welcome(&msg, 1, "test server", NULL, 0) );
		}
		
		if (server->client_disconnected_private != NULL){
			printf("client disconnected: %p\n", server->client_disconnected_private);
		}
		
		//SDL_WaitEvent(NULL);
		
		/*
		ipc_client_p client = NULL;
		message_p msg = ipc_server_fetch(server, 50, &client);
		switch(msg->type){
			case MSG_HELLO:
				
				msg = (message_t){ MSG_WELCOME, seq, .welcome_resp = { 1, 0x0000000000000000 } };
				strncpy(msg.welcome_resp.server_name, "test server", 32);
				ipc_server_send(server, msg);
				break;
		}
		*/
		
		while ( SDL_PollEvent(&e) ) {
			switch(e.type){
				case SDL_QUIT:
					quit = true;
					break;
				case SDL_VIDEORESIZE:
					renderer_resize(e.resize.w, e.resize.h);
					break;
				case SDL_KEYUP:
					switch(e.key.keysym.sym){
						case SDLK_LEFT:
							viewport->pos.x -= 1;
							vp_changed(viewport);
							break;
						case SDLK_RIGHT:
							viewport->pos.x += 1;
							vp_changed(viewport);
							break;
						case SDLK_UP:
							viewport->pos.y += 1;
							vp_changed(viewport);
							break;
						case SDLK_DOWN:
							viewport->pos.y -= 1;
							vp_changed(viewport);
							break;
						default:
							ipc_server_broadcast(server, msg_keyup(&msg, e.key.keysym.sym, e.key.keysym.mod));
							break;
					}
					break;
				case SDL_KEYDOWN:
					ipc_server_broadcast(server, msg_keydown(&msg, e.key.keysym.sym, e.key.keysym.mod));
					break;
				case SDL_MOUSEMOTION:
					cursor_pos.x = e.motion.x;
					cursor_pos.y = e.motion.y;
					
					//vec2_t world_cursor = m3_v2_mul(viewport->screen_to_world, cursor_pos);
					//printf("world cursor: %f %f\n", world_cursor.x, world_cursor.y);
					//particles[0].pos = world_cursor;
					
					if (viewport_grabbed) {
						// Only use the scaling factors from the current screen to world matrix. Since we work
						// with deltas here the offsets are not necessary (in fact would destroy the result).
						viewport->pos.x += -viewport->screen_to_world[0] * e.motion.xrel;
						viewport->pos.y += -viewport->screen_to_world[4] * e.motion.yrel;
						vp_changed(viewport);
					} else {
						ipc_server_broadcast(server, msg_mouse_motion(&msg, e.motion.state, e.motion.x, e.motion.y));
					}
					
					break;
				case SDL_MOUSEBUTTONDOWN:
					switch(e.button.button){
						//case SDL_BUTTON_LEFT:
						//	break;
						case SDL_BUTTON_MIDDLE:
							viewport_grabbed = true;
							break;
						//case SDL_BUTTON_RIGHT:
						//	break;
						//case SDL_BUTTON_WHEELUP:
						//	break;
						//case SDL_BUTTON_WHEELDOWN:
						//	break;
						default:
							ipc_server_broadcast(server, msg_mouse_button(&msg, e.button.type, e.button.which, e.button.button, e.button.state, e.button.x, e.button.y));
							break;
					}
					break;
				case SDL_MOUSEBUTTONUP:
					switch(e.button.button){
						//case SDL_BUTTON_LEFT:
						//	break;
						case SDL_BUTTON_MIDDLE:
							viewport_grabbed = false;
							break;
						//case SDL_BUTTON_RIGHT:
						//	break;
						case SDL_BUTTON_WHEELUP:
							viewport->scale_exp -= 1;
							{
								vec2_t world_cursor = m3_v2_mul(viewport->screen_to_world, cursor_pos);
								float new_scale = vp_scale_for(viewport, viewport->scale_exp);
								viewport->pos = (vec2_t){
									world_cursor.x + (viewport->pos.x - world_cursor.x) * (new_scale / viewport->scale),
									world_cursor.y + (viewport->pos.y - world_cursor.y) * (new_scale / viewport->scale)
								};
							}
							vp_changed(viewport);
							break;
						case SDL_BUTTON_WHEELDOWN:
							viewport->scale_exp += 1;
							{
								vec2_t world_cursor = m3_v2_mul(viewport->screen_to_world, cursor_pos);
								float new_scale = vp_scale_for(viewport, viewport->scale_exp);
								viewport->pos = (vec2_t){
									world_cursor.x + (viewport->pos.x - world_cursor.x) * (new_scale / viewport->scale),
									world_cursor.y + (viewport->pos.y - world_cursor.y) * (new_scale / viewport->scale)
								};
							}
							vp_changed(viewport);
							break;
						default:
							ipc_server_broadcast(server, msg_mouse_button(&msg, e.button.type, e.button.which, e.button.button, e.button.state, e.button.x, e.button.y));
							break;
					}
					break;
			}
		}
		
		
		if (trigger_redraw){
			// If we have any retained draw requests left we need to clean them up before starting new stuff
			if (retained_draw_requests) {
				// TODO: Merge retained draw request buffers into any current draw request buffers
				// Free retained draw requests (shared memory files and tree)
				int free_iterator(draw_request_tree_p node){
					close(node->value.shm_fd);
					return NULL;
				}
				draw_request_tree_iterate(retained_draw_requests, free_iterator);
				draw_request_tree_destroy(retained_draw_requests);
			}
			
			// Remember any draw requests that are still running in case something was late or
			// we interrupted a running draw operation.
			retained_draw_requests = draw_requests;
			
			// Put draw requests for all visible layers into the message buffers
			int64_t x, y;
			uint64_t w, h;
			vp_vis_world_rect(viewport, &x, &y, &w, &h);
			draw_requests = layers_in_rect(x, y, w, h, viewport->scale_exp);
			
			int send_iterator(draw_request_tree_p node){
				// Create a shared memory file and delete it directly afterwards. We just
				// want the file descriptor and as long as it is open the file data is not
				// deleted.
				const char *shm_name = "/plains_layer";
				int fd = shm_open(shm_name, O_RDWR | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR);
				if (fd == -1){
					perror("shm_open() failed");
					return NULL;
				}
				
				if ( shm_unlink(shm_name) == -1 )
					perror("shm_unlink() failed");
				
				size_t shm_size = node->value.w * node->value.h * 4;
				if ( ftruncate(fd, shm_size) == -1 )
					perror("ftruncate() on shared memory failed");
				
				// Send fd to client to render the stuff into. Then wait for the status response.
				msg_t msg;
				node->value.req_seq = ipc_server_send_with_fd(server, layer->client_idx,
					msg_draw(&msg, 0, layer->private, 0, 0, 0, scale->width, scale->height, scale->scale_index, 0.0),
					fd);
				
				return NULL;
			}
			draw_request_tree_iterate(send_iterator);
			
			// Clear back buffer and draw grid
			renderer_clear();
			grid_draw();
			
			trigger_redraw = false;
		}
		
		
		// Process all incomming messages from clients
		for(size_t i = 0; i < server->client_count; i++){
			ipc_client_p client = &server->clients[i];
			
			size_t buffer_size = 0;
			void *buffer = NULL;
			while( (buffer = msg_queue_start_dequeue(&client->in, &buffer_size)) != NULL ){
				printf("client %zu: ", i);
				
				msg_deserialize(&msg, buffer, buffer_size);
				msg_print(&msg);
				switch(msg.type){
					case MSG_LAYER_CREATE: {
						size_t layer_id = layer_new(msg.layer_create.x, msg.layer_create.y, msg.layer_create.z, msg.layer_create.width, msg.layer_create.height, msg.layer_create.private);
						ipc_server_send(server, i, msg_status(&msg, msg.seq, 0, layer_id) );
						} break;
					case MSG_STATUS: {
						// Search for draw request with a matching req_seq
						// TODO: Would be more efficient to keep a list of expected response seq in the client structure
						int req_search_iterator(draw_request_tree_p node){
							if (node->req_seq == msg->status.seq)
								return node;
							return NULL;
						}
						draw_request_tree_p req_node = draw_request_tree_iterate(draw_requests, req_search_iterator);
						// Ignore the status response if there is no matching draw request
						if (req_node == NULL)
							break;
						
						if (msg->status.status == 2) {
							// Draw request failed, draw nothing, free shared memory and mark
							// request as done (but not answered). There is no need to keep the
							// shared memory buffer around that has no useful contents.
							close(req_node->value.shm_fd);
							req_node->value.shm_fd = -1;
							req_node->value.flags |= DRAW_REQUEST_DONE;
						} else if (msg->status.status == 0 || msg->status.status == 1) {
							if (msg->status.status == 1) {
								// Draw request failed but we should draw a placeholder. Free
								// the shared memory since it contains nothing useful.
								req_node->value.flags |= DRAW_REQUEST_PLACEHOLDER;
								close(req_node->value.shm_fd);
								req_node->value.shm_fd = -1;
							} else {
								// Draw request successfully answered, the buffer contains
								// the pixel data we want to draw.
								req_node->value.flags |= DRAW_REQUEST_ANSWERED;
							}
							
							// Check if all draw requests we depend on are done yet
							bool blocked = false;
							for(draw_request_tree_p node = req_node->parent; node != draw_requests; node = node->parent){
								if ( !(node->value.flags & DRAW_REQUEST_DONE) ){
									blocked = true;
									break;
								}
							}
							
							// If we're blocked we need to wait til the parent is drawn. After
							// that we will be drawn.
							if (blocked)
								break;
							
							// We're not blocked, get the pixels drawn
							renderer_draw_response(&req_node->value);
							
							// Draw any dependent requests that are now unblocked
							int draw_iterator(draw_request_tree_p node){
								// Theoretically we could skip the DONE check since child requests
								// can not be done before all parents (dependencies)... but well, just to be sure...
								if ( !(node->value.flags & DRAW_REQUEST_DONE) && ((node->value.flags & DRAW_REQUEST_ANSWERED) || (node->value.flags & DRAW_REQUEST_PLACEHOLDER)) )
									renderer_draw_response(&req_node->value);
								return NULL;
							}
							draw_request_tree_iterate(req_node, draw_iterator);
							
							// Free the draw request tree if all requests are served
							size_t pending_draw_requests = 0;
							int count_iterator(draw_request_tree_p node){
								if ( !(node->value.flags & DRAW_REQUEST_DONE) )
									pending_draw_requests++;
								return NULL;
							}
							draw_request_tree_iterate(draw_requests, count_iterator);
							
							if (pending_draw_requests == 0){
								render_finish_draw();
								
								int free_iterator(draw_request_tree_p node){
									close(node->value.shm_fd);
									return NULL;
								}
								draw_request_tree_iterate(draw_requests, free_iterator);
								
								draw_request_tree_destroy(draw_requests);
							}

						} else {
							fprintf("unknown draw response status: %u\n", msg->status.status);
							break;
						}
						
						} break;
				}
				
				msg_queue_end_dequeue(&client->in, buffer);
			}
		}
	}
	
	// Cleanup time
	ipc_server_destroy(server);
	tile_table_destroy(tile_table);
	
	debug_unload();
	layers_unload();
	cursor_unload();
	grid_unload();
	renderer_unload();
	
	SDL_Quit();
	return 0;
}