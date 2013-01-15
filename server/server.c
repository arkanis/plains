#include <stdio.h>
#include <stdbool.h>

#include "object_tree.h"
#include "viewport.h"
#include "renderer.h"

int main(int argc, char **argv){
	renderer_p renderer = renderer_new(800, 800, "Plains");
	object_tree_p world = object_tree_new((object_t){});
	viewport_p viewport = vp_new(800, 800, 2.0, 0.0);
	
	object_tree_append(world, (object_t){0,   0,   0, 100, 100});
	object_tree_p objA = object_tree_append(world, (object_t){150, 0,   0, 50, 100});
		object_tree_p objB = object_tree_append(objA, (object_t){10, 10, 0, 40, 40});
			object_tree_append(objB, (object_t){10, 10, 0, 10, 10});
	object_tree_append(world, (object_t){0,   150, 0, 100, 50});
	
	SDL_Event e;
	bool quit = false, viewport_grabbed = false, redraw = true;
	
	while (!quit) {
		// Wait until events are in the queue
		SDL_WaitEvent(NULL);
		
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
							viewport->pos.x -= 25;
							vp_changed(viewport);
							redraw = true;
							break;
						case SDLK_RIGHT:
							viewport->pos.x += 25;
							vp_changed(viewport);
							redraw = true;
							break;
						case SDLK_UP:
							viewport->pos.y += 25;
							vp_changed(viewport);
							redraw = true;
							break;
						case SDLK_DOWN:
							viewport->pos.y -= 25;
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
					if (viewport_grabbed) {
						// Only use the scaling factors from the current screen to world matrix. Since we work
						// with deltas here the offsets are not necessary (in fact would destroy the result).
						//printf("scale %f, %f, rel %d, %d\n", viewport->screen_to_world[0], viewport->screen_to_world[4], e.motion.xrel, e.motion.yrel);
						viewport->pos.x += -viewport->screen_to_world[0] * e.motion.xrel;
						viewport->pos.y += viewport->screen_to_world[4] * e.motion.yrel;
						vp_changed(viewport);
						redraw = true;
					}
					break;
				case SDL_MOUSEBUTTONDOWN:
					switch(e.button.button){
						case SDL_BUTTON_LEFT:
							viewport_grabbed = true;
							break;
						case SDL_BUTTON_MIDDLE:
							break;
						case SDL_BUTTON_RIGHT:
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
							viewport_grabbed = false;
							break;
						case SDL_BUTTON_MIDDLE:
							break;
						//case SDL_BUTTON_RIGHT:
						//	break;
						case SDL_BUTTON_WHEELUP:
							viewport->scale_exp -= 1;
							{
								vec2_t world_cursor = m3_v2_mul(viewport->screen_to_world, (vec2_t){e.button.x, e.button.y});
								//printf("cursor %d, %d, world: %f, %f\n", e.button.x, e.button.y, world_cursor.x, world_cursor.y);
								float new_scale = vp_scale_for(viewport, viewport->scale_exp);
								viewport->pos = (ivec2_t){
									world_cursor.x + (viewport->pos.x - world_cursor.x) * (new_scale / viewport->scale),
									world_cursor.y + (viewport->pos.y - world_cursor.y) * (new_scale / viewport->scale)
								};
							}
							vp_changed(viewport);
							redraw = true;
							break;
						case SDL_BUTTON_WHEELDOWN:
							viewport->scale_exp += 1;
							{
								vec2_t world_cursor = m3_v2_mul(viewport->screen_to_world, (vec2_t){e.button.x, e.button.y});
								//printf("cursor %d, %d, world: %f, %f\n", e.button.x, e.button.y, world_cursor.x, world_cursor.y);
								float new_scale = vp_scale_for(viewport, viewport->scale_exp);
								viewport->pos = (ivec2_t){
									world_cursor.x + (viewport->pos.x - world_cursor.x) * (new_scale / viewport->scale),
									world_cursor.y + (viewport->pos.y - world_cursor.y) * (new_scale / viewport->scale)
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
			//draw_request_tree_p req_tree = draw_request_tree_new((draw_request_t){});
			
			renderer_clear(renderer);
			irect_t screen_rect = vp_vis_world_rect(viewport);
			screen_rect.x += 100; screen_rect.y += 100;
			screen_rect.w -= 200; screen_rect.h -= 200;
			
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
				
				draw_request_t req = (draw_request_t){ .x = x, .y = y, .object = &node->value, .color = (color_t){0, 0, 1, 0.5} };
				renderer_draw_response(renderer, viewport, &req);
				return NULL;
			}
			object_tree_iterate(world, draw_iterator);
			render_finish_draw(renderer);
			
			redraw = false;
		}
	}
	
	vp_destroy(viewport);
	object_tree_destroy(world);
	renderer_destroy(renderer);
	
	return 0;
}