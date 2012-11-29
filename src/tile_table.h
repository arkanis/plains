#pragma once

#include <stdint.h>

typedef struct {
	// Side length (width and height) of one tile in pixels
	uint16_t tile_size;
	// Texture dimensions in pixels
	uint32_t width, height;
	// OpenGL texture object
	GLuint texture;
} tile_table_t, *tile_table_p;

tile_table_p tile_table_new(uint32_t texture_width, uint32_t texture_height, uint16_t tile_size);
void tile_table_destroy(tile_table_p tile_table);