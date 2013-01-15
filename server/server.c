#include <stdio.h>
#include <stdbool.h>

#include "object_tree.h"
#include "viewport.h"
#include "renderer.h"

int main(int argc, char **argv){
	renderer_p renderer = renderer_new(640, 480, "Plains");
	object_tree_p world = object_tree_new((object_t){});
	viewport_p viewport = vp_new((ivec2_t){640, 480}, 2.0, 0.0);
	
	object_tree_append(world, (object_t){0,   0,   0, 100, 100});
	object_tree_append(world, (object_t){150, 0,   0, 50, 100});
	object_tree_append(world, (object_t){0,   150, 0, 100, 50});
	
	SDL_Event e;
	bool quit = false, viewport_grabbed = false, redraw = true;
	
	while ( SDL_WaitEvent(&e) ) {
		switch(e.type){
			case SDL_QUIT:
				quit = true;
				break;
			case SDL_VIDEORESIZE:
				renderer_resize(renderer, e.resize.w, e.resize.h);
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
						viewport->pos.y += 1;
						vp_changed(viewport);
						redraw = true;
						break;
					case SDLK_DOWN:
						viewport->pos.y -= 1;
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
					viewport->pos.y += -viewport->screen_to_world[4] * e.motion.yrel;
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
		
		if (redraw) {
			//draw_request_tree_p req_tree = draw_request_tree_new((draw_request_t){});
			
			renderer_clear(renderer);
			object_tree_p draw_iterator(object_tree_p node){
				draw_request_t req = (draw_request_t){ .object = &node->value, .color = (color_t){0, 0, 1, 0.5} };
				renderer_draw_response(renderer, viewport, &req);
				return NULL;
			}
			object_tree_iterate(world, draw_iterator);
			render_finish_draw(renderer);
			
			redraw = false;
		}
		
		if (quit)
			break;
	}
	
	vp_destroy(viewport);
	object_tree_destroy(world);
	renderer_destroy(renderer);
	
	return 0;
}