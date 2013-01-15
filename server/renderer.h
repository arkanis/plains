#pragma once

#include <SDL/SDL.h>
#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>

#include "draw_request.h"
#include "viewport.h"


typedef struct {
	GLuint texture, prog, vertex_buffer;
} renderer_t, *renderer_p;


renderer_p renderer_new(uint16_t window_width, uint16_t window_height, const char *title);
void renderer_destroy(renderer_p renderer);

void renderer_resize(renderer_p renderer, uint16_t window_width, uint16_t window_height);

void renderer_clear(renderer_p renderer);
void renderer_draw_response(renderer_p renderer, viewport_p viewport, draw_request_p req);
void render_finish_draw(renderer_p renderer);