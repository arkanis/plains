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

#include "common.h"
#include "math.h"
#include "viewport.h"
#include "layer.h"
#include "tile_table.h"

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
}

void debug_unload(){
	glDeleteBuffers(1, &debug_vertex_buffer);
	delete_program_and_shaders(debug_prog);
}

/**
 * Renders the tile table texture at the origin.
 */
void debug_draw(){
	uint32_t x = 0, y = 0;
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

void renderer_draw(){
	glClearColor(1, 1, 1, 1.0);
	glClear(GL_COLOR_BUFFER_BIT);
	
	grid_draw();
	layers_draw(viewport);
	cursor_draw();
	debug_draw();
}




int main(int argc, char **argv){
	uint16_t win_w = 640, win_h = 480;
	
	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER);
	renderer_load(win_w, win_h, "Grid");
	
	grid_load();
	cursor_load();
	layers_load();
	debug_load();
	
	layer_new(0, 0, 1, image_haruhi.width, image_haruhi.height, image_haruhi.pixel_data);
	layer_new(0, 500, 0, image_window.width, image_window.height, image_window.pixel_data);
	layer_new(0, -500, 0, image_window.width, image_window.height, image_window.pixel_data);
	
	tile_table = tile_table_new(2048, 2048, 128);
	
	SDL_Event e;
	bool quit = false, viewport_grabbed = false;
	
	while (!quit) {
		SDL_WaitEvent(NULL);
		
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
					}
					break;
				case SDL_MOUSEMOTION:
					cursor_pos.x = e.motion.x;
					cursor_pos.y = e.motion.y;
					
					//vec2_t world_cursor = m3_v2_mul(viewport->screen_to_world, cursor_pos);
					//printf("world cursor: %f %f\n", world_cursor.x, world_cursor.y);
					//particles[0].pos = world_cursor;
					
					if (viewport_grabbed){
						// Only use the scaling factors from the current screen to world matrix. Since we work
						// with deltas here the offsets are not necessary (in fact would destroy the result).
						viewport->pos.x += -viewport->screen_to_world[0] * e.motion.xrel;
						viewport->pos.y += -viewport->screen_to_world[4] * e.motion.yrel;
						vp_changed(viewport);
					}
					
					break;
				case SDL_MOUSEBUTTONDOWN:
					switch(e.button.button){
						case SDL_BUTTON_LEFT:
							break;
						case SDL_BUTTON_MIDDLE:
							viewport_grabbed = true;
							break;
						case SDL_BUTTON_RIGHT:
							break;
						case SDL_BUTTON_WHEELUP:
							break;
						case SDL_BUTTON_WHEELDOWN:
							break;
					}
					break;
				case SDL_MOUSEBUTTONUP:
					switch(e.button.button){
						case SDL_BUTTON_LEFT:
							break;
						case SDL_BUTTON_MIDDLE:
							viewport_grabbed = false;
							break;
						case SDL_BUTTON_RIGHT:
							break;
						case SDL_BUTTON_WHEELUP:
							viewport->scale_exp -= 0.1;
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
							viewport->scale_exp += 0.1;
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
					}
					break;
			}
		}
		
		renderer_draw();
		SDL_GL_SwapBuffers();
	}
	
	// Cleanup time
	tile_table_destroy(tile_table);
	
	debug_unload();
	layers_unload();
	cursor_unload();
	grid_unload();
	renderer_unload();
	
	SDL_Quit();
	return 0;
}