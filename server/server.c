#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "object_tree.h"
#include "draw_request_tree.h"
#include "viewport.h"
#include "renderer.h"
#include "ipc_server.h"


draw_request_tree_p build_draw_request_tree(object_tree_p world, viewport_p viewport);
void check_for_empty_draw_tree_and_finish_rendering(draw_request_tree_p* draw_requests, renderer_p renderer);
object_tree_p find_hit_object(object_tree_p world, ivec2_t pos);


int main(int argc, char **argv){
	renderer_p renderer = renderer_new(800, 800, "Plains");
	object_tree_p world = object_tree_new((object_t){});
	viewport_p viewport = vp_new(800, 800, 2.0, 0.0);
	ipc_server_p server = ipc_server_new("server.socket", 3);
	
	object_tree_p grabbed = NULL;
	
	/*
	object_tree_append(world, (object_t){0,   0,   0, 100, 100, 999});
	object_tree_p objA = object_tree_append(world, (object_t){150, 0,   0, 50, 100, 999});
		object_tree_p objB = object_tree_append(objA, (object_t){10, 10, 0, 40, 40, 999});
			object_tree_append(objB, (object_t){10, 10, 0, 10, 10, 999});
	object_tree_append(world, (object_t){-5,   150, 0, 100, 50, 999});
	
	object_tree_p scursor = object_tree_append(viewport->screen_space_objects, (object_t){0, 0, 0, 10, 10, 999});
	//object_tree_p wcursor = object_tree_append(world, (object_t){0,   0,   0, 20, 20, 999});
	//object_tree_p wview = object_tree_append(world, (object_t){0,   0,   0, 1, 1, 999});
	*/
	
	draw_request_tree_p draw_requests = NULL, retained_draw_requests = NULL;
	SDL_Event e;
	bool quit = false, viewport_grabbed = false, redraw = true;
	
	while (!quit) {
		void connect_handler(size_t client_idx, ipc_client_p client){
			printf("client connected %zu...\n", client_idx);
		}
		
		void disconnect_handler(size_t client_idx, ipc_client_p client){
			printf("client disconnected %zu...\n", client_idx);
			
			uint8_t client_cleaner(object_tree_p node){
				return (node->value.client_idx == client_idx);
			}
			object_tree_delete(world, client_cleaner);
			
			redraw = true;
		}
		
		void recv_handler(size_t client_idx, ipc_client_p client, plains_msg_p msg){
			//plains_msg_print(msg);
			switch(msg->type){
				case PLAINS_MSG_OBJECT_CREATE: {
					object_tree_p obj = object_tree_append(world, (object_t){
						msg->object_create.x, msg->object_create.y, msg->object_create.z,
						msg->object_create.width, msg->object_create.height,
						client_idx, msg->object_create.private
					});
					redraw = true;
					
					plains_msg_t resp;
					ipc_server_send(client, msg_status(&resp, msg->seq, 0, (uint64_t)obj) );
					} break;
				case PLAINS_MSG_OBJECT_UPDATE: {
					// TODO: Big fat security nightmare...
					object_tree_p obj = (object_tree_p)msg->object_update.object_id;
					obj->value = (object_t){
						msg->object_update.x, msg->object_update.y, msg->object_update.z,
						msg->object_update.width, msg->object_update.height,
						client_idx, msg->object_update.private
					};
					redraw = true;
					
					plains_msg_t resp;
					ipc_server_send(client, msg_status(&resp, msg->seq, 0, 0) );
					} break;
				case PLAINS_MSG_STATUS: {
					// Search for draw request with a matching req_seq. We ignore the status
					// response if there is no matching draw request.
					// TODO: Would be more efficient to keep a list of expected response seq
					// in the client structure.
					draw_request_tree_p req_search_iterator(draw_request_tree_p node){
						if (node->value.req_seq == msg->status.seq)
							return node;
						return NULL;
					}
					draw_request_tree_p req_node = draw_request_tree_iterate(draw_requests, req_search_iterator);
					if (req_node == NULL)
						break;
					
					if (msg->status.status == 0) {
						// Draw request successful, the buffer contains the pixel data we want
						// to draw.
						req_node->value.flags |= DRAW_REQUEST_BUFFERED;
						
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
						renderer_draw_response(renderer, &req_node->value);
						req_node->value.flags |= DRAW_REQUEST_DONE;
						
						// Draw any dependent requests that are now unblocked
						draw_request_tree_p draw_iterator(draw_request_tree_p node){
							// Theoretically we could skip the DONE check since child requests
							// can not be done before all parents (dependencies)... but well, just to be sure...
							if ( !(node->value.flags & DRAW_REQUEST_DONE) && (node->value.flags & DRAW_REQUEST_BUFFERED) )
								renderer_draw_response(renderer, &req_node->value);
							return NULL;
						}
						draw_request_tree_iterate(req_node, draw_iterator);
						
						// Free the draw request tree if all requests are served
						check_for_empty_draw_tree_and_finish_rendering(&draw_requests, renderer);
					} else {
						// Draw request failed, draw nothing, free shared memory and mark
						// request as done (but not buffered). There is no need to keep the
						// shared memory buffer around that has no useful contents.
						close(req_node->value.shm_fd);
						req_node->value.shm_fd = -1;
						req_node->value.flags |= DRAW_REQUEST_DONE;
					}
					
					} break;
			}
		}
		
		ipc_server_cycle(server, 10, recv_handler, connect_handler, disconnect_handler);
		
		// If we have events ready process them in one batch
		while ( SDL_PollEvent(&e) ) {
			switch(e.type){
				case SDL_QUIT:
					quit = true;
					break;
				case SDL_VIDEORESIZE:
					vp_screen_changed(viewport, e.resize.w, e.resize.h);
					renderer_resize(renderer, e.resize.w, e.resize.h);
					redraw = true;
					break;
				case SDL_KEYUP:
					switch(e.key.keysym.sym){
						case SDLK_LEFT:
							viewport->pos.x -= 1;
							vp_changed(viewport);
							redraw = true;
							break;
						case SDLK_RIGHT:
							viewport->pos.x += 1;
							vp_changed(viewport);
							redraw = true;
							break;
						case SDLK_UP:
							viewport->pos.y -= 1;
							vp_changed(viewport);
							redraw = true;
							break;
						case SDLK_DOWN:
							viewport->pos.y += 1;
							vp_changed(viewport);
							redraw = true;
							break;
						default:
							break;
					}
					break;
				case SDL_KEYDOWN:
					break;
				case SDL_MOUSEMOTION:
					/*
					{
						vec2_t world_cursor = m3_v2_mul(viewport->screen_to_world, (vec2_t){e.button.x, e.button.y});
						if (world->first){
							world->first->value.x = world_cursor.x;
							world->first->value.y = world_cursor.y;
						}
						printf("world_cursor: %f %f, %lu %lu\n", world_cursor.x, world_cursor.y, world->first->value.width, world->first->value.height);
					}
					scursor->value.x = e.button.x;
					scursor->value.y = e.button.y;
					
					redraw = true;
					*/
					
					if (viewport_grabbed) {
						// Only use the scaling factors from the current screen to world matrix. Since we work
						// with deltas here the offsets are not necessary (in fact would destroy the result).
						// We also invert the position differences so the world moves along the cursor (not in
						// the opposite direction).
						//printf("scale %f, %f, rel %d, %d\n", viewport->screen_to_world[0], viewport->screen_to_world[4], e.motion.xrel, e.motion.yrel);
						float dx = viewport->subpixel_pos.x + -viewport->screen_to_world[0] * e.motion.xrel;
						float dy = viewport->subpixel_pos.y + -viewport->screen_to_world[4] * e.motion.yrel;
						float dxi, dyi;
						dx = modff(dx, &dxi);
						dy = modff(dy, &dyi);
						
						viewport->pos.x += dxi;
						viewport->pos.y += dyi;
						viewport->subpixel_pos.x = dx;
						viewport->subpixel_pos.y = dy;
						
						vp_changed(viewport);
						redraw = true;
					} else if (grabbed) {
						object_p obj = &grabbed->value;
						ivec2_t world_pos = vp_screen_to_world_pos(viewport, e.motion.x, e.motion.y);
						
						ipc_client_p client = &server->clients[obj->client_idx];
						plains_msg_t msg;
						ipc_server_send(client, msg_mouse_motion(&msg,
							(uint64_t)obj, obj->client_private, 1,
							e.motion.state, obj->x - world_pos.x, obj->y - world_pos.y,
							e.motion.xrel / viewport->scale, e.motion.yrel / viewport->scale
						));
					} else {
						ivec2_t world_pos = vp_screen_to_world_pos(viewport, e.motion.x, e.motion.y);
						//printf("world_pos: %ld %ld\n", world_pos.x, world_pos.y);
						object_tree_p hit_object_node = find_hit_object(world, world_pos);
						
						if (hit_object_node) {
							object_p obj = &hit_object_node->value;
							
							ipc_client_p client = &server->clients[obj->client_idx];
							plains_msg_t msg;
							ipc_server_send(client, msg_mouse_motion(&msg,
								(uint64_t)obj, obj->client_private, 1,
								e.motion.state, obj->x - world_pos.x, obj->y - world_pos.y,
								e.motion.xrel / viewport->scale, e.motion.yrel / viewport->scale
							));
						}
					}
					break;
				case SDL_MOUSEBUTTONDOWN:
					switch(e.button.button){
						case SDL_BUTTON_LEFT: {
							ivec2_t world_pos = vp_screen_to_world_pos(viewport, e.button.x, e.button.y);
							grabbed = find_hit_object(world, world_pos);
							} break;
						case SDL_BUTTON_MIDDLE:
							break;
						case SDL_BUTTON_RIGHT:
							viewport_grabbed = true;
							break;
						case SDL_BUTTON_WHEELUP:
							break;
						case SDL_BUTTON_WHEELDOWN:
							break;
						default:
							break;
					}
					break;
				case SDL_MOUSEBUTTONUP:
					switch(e.button.button){
						case SDL_BUTTON_LEFT:
							grabbed = NULL;
							break;
						case SDL_BUTTON_MIDDLE:
							break;
						case SDL_BUTTON_RIGHT:
							viewport_grabbed = false;
							break;
						case SDL_BUTTON_WHEELUP:
							viewport->scale_exp += 1;
							{
								vec2_t world_cursor = m3_v2_mul(viewport->screen_to_world, (vec2_t){e.button.x, e.button.y});
								//printf("cursor %d, %d, world: %f, %f\n", e.button.x, e.button.y, world_cursor.x, world_cursor.y);
								float new_scale = vp_scale_for(viewport, viewport->scale_exp);
								viewport->pos = (ivec2_t){
									world_cursor.x + (viewport->pos.x - world_cursor.x) * (viewport->scale / new_scale),
									world_cursor.y + (viewport->pos.y - world_cursor.y) * (viewport->scale / new_scale)
								};
							}
							vp_changed(viewport);
							redraw = true;
							break;
						case SDL_BUTTON_WHEELDOWN:
							viewport->scale_exp -= 1;
							{
								vec2_t world_cursor = m3_v2_mul(viewport->screen_to_world, (vec2_t){e.button.x, e.button.y});
								//printf("cursor %d, %d, world: %f, %f\n", e.button.x, e.button.y, world_cursor.x, world_cursor.y);
								float new_scale = vp_scale_for(viewport, viewport->scale_exp);
								viewport->pos = (ivec2_t){
									world_cursor.x + (viewport->pos.x - world_cursor.x) * (viewport->scale / new_scale),
									world_cursor.y + (viewport->pos.y - world_cursor.y) * (viewport->scale / new_scale)
								};
							}
							vp_changed(viewport);
							redraw = true;
							break;
						default:
							break;
					}
					break;
			}
		}
		
		
		if (redraw) {
			// If we have any retained draw requests left we need to clean them up before starting new stuff
			if (retained_draw_requests) {
				// TODO: Merge retained draw request buffers into any current draw request buffers
				// Free retained draw requests (shared memory files and tree)
				draw_request_tree_p free_iterator(draw_request_tree_p node){
					close(node->value.shm_fd);
					return NULL;
				}
				draw_request_tree_iterate(retained_draw_requests, free_iterator);
				draw_request_tree_destroy(retained_draw_requests);
			}
			
			// Remember any draw requests that are still running in case something was late or
			// we interrupted a running draw operation.
			retained_draw_requests = draw_requests;
			
			// Send out draw requests for all visible objects
			draw_requests = build_draw_request_tree(world, viewport);
			
			draw_request_tree_p send_iterator(draw_request_tree_p node){
				draw_request_p req = &node->value;
				
				// Create a shared memory file and delete it directly afterwards. We just
				// want the file descriptor. As long as it is open the file data is not
				// deleted.
				const char *shm_name = "/plains_object";
				int fd = shm_open(shm_name, O_RDWR | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR);
				if (fd == -1){
					perror("shm_open() failed");
					return NULL;
				}
				
				if ( shm_unlink(shm_name) == -1 )
					perror("shm_unlink() failed");
				
				req->shm_fd = fd;
				size_t shm_size = req->rw * req->rh * 4;
				if ( ftruncate(fd, shm_size) == -1 )
					perror("ftruncate() on shared memory failed");
				
				// Send fd to client to render the stuff into. Then wait for the status response.
				size_t client_idx = req->object->client_idx;
				ipc_client_p client = &server->clients[client_idx];
				plains_msg_t msg;
				int err = ipc_server_send(client, msg_draw(&msg,
					(uint64_t)req->object, req->object->client_private,
					req->rx, req->ry, req->rw, req->rh,
					req->scale, req->bw, req->bh, req->shm_fd
				));
				
				// If we failed to send the request mark it done so it does not block other requests
				if (err < 0)
					req->flags |= DRAW_REQUEST_DONE;
				else
					req->req_seq = msg.seq;
				
				return NULL;
			}
			draw_request_tree_iterate(draw_requests, send_iterator);
			
			// Clear back buffer
			renderer_clear(renderer);
			
			check_for_empty_draw_tree_and_finish_rendering(&draw_requests, renderer);
			
			redraw = false;
		}
	}
	
	ipc_server_destroy(server);
	vp_destroy(viewport);
	object_tree_destroy(world);
	renderer_destroy(renderer);
	
	return 0;
}

draw_request_tree_p build_draw_request_tree(object_tree_p world, viewport_p viewport){
	draw_request_tree_p tree = draw_request_tree_new((draw_request_t){});
	irect_t screen_rect = vp_vis_world_rect(viewport);
	float scale = viewport->scale;
	
	object_tree_p draw_iterator(object_tree_p node){
		// Calculate non nested world coords
		int64_t x = node->value.x, y = node->value.y;
		for(object_tree_p n = node->parent; n; n = n->parent){
			x += n->value.x;
			y += n->value.y;
		}
		
		// Check if it is visible
		irect_t i = irect_intersection((irect_t){x, y, node->value.width, node->value.height}, screen_rect);
		if( i.w == 0 || i.h == 0 )
			return NULL;
		
		// Only scale buffer to smaller levels. There is no use in letting
		// the client render stuff more accurate than we can display on the
		// screen.
		float buffer_scale = (scale < 1) ? scale : 1;
		draw_request_tree_append(tree, (draw_request_t){
			.object = &node->value,
			.wx = i.x, .wy = i.y, .ww = i.w, .wh = i.h,
			.transform = &viewport->world_to_normal[0],
			.bw = i.w * buffer_scale, .bh = i.h * buffer_scale,
			
			.scale = scale,
			.rx = imax(screen_rect.x - x, 0), .ry = imax(screen_rect.y - y, 0),
			.rw = i.w, .rh = i.h,
			
			.color = (color_t){0, 0, 1, 0.5},
			.shm_fd = -1, .req_seq = 0, .flags = 0
		});
		
		return NULL;
	}
	object_tree_iterate(world, draw_iterator);
	
	return tree;
}

void check_for_empty_draw_tree_and_finish_rendering(draw_request_tree_p* draw_requests, renderer_p renderer){
	size_t pending_draw_requests = 0;
	draw_request_tree_p count_iterator(draw_request_tree_p node){
		if ( !(node->value.flags & DRAW_REQUEST_DONE) )
			pending_draw_requests++;
		return NULL;
	}
	draw_request_tree_iterate(*draw_requests, count_iterator);
	
	if (pending_draw_requests == 0){
		render_finish_draw(renderer);
		
		draw_request_tree_p free_iterator(draw_request_tree_p node){
			if (node->value.shm_fd != -1)
				close(node->value.shm_fd);
			return NULL;
		}
		draw_request_tree_iterate(*draw_requests, free_iterator);
		
		draw_request_tree_destroy(*draw_requests);
		*draw_requests = NULL;
	}
}

// TODO: Take care of hit tree and event probagation
object_tree_p find_hit_object(object_tree_p world, ivec2_t pos){
	object_tree_p find_iterator(object_tree_p node){
		// Calculate non nested world coords
		int64_t x = node->value.x, y = node->value.y;
		for(object_tree_p n = node->parent; n; n = n->parent){
			x += n->value.x;
			y += n->value.y;
		}
		
		// Casts are required for C to properly sign extend... damn
		if (pos.x >= x && pos.x < x + (int64_t)node->value.width && pos.y >= y && pos.y < y + (int64_t)node->value.height)
			return node;
		return NULL;
	}
	// We use post iteration here (iterate parent after children) so we get
	// the deepest (most accurate) hit.
	return object_tree_iterate_post(world, find_iterator);
}