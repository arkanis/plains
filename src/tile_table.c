#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>

#include "tile_table.h"


/**
 * Creates a new tile table and allocates an OpenGL texture for it.
 */
tile_table_p tile_table_new(uint32_t texture_width, uint32_t texture_height, uint16_t tile_size){
	tile_table_p tt = malloc(sizeof(tile_table_t));
	
	// Store dimensions
	assert(texture_width % tile_size == 0 && texture_height % tile_size == 0);
	tt->width = texture_width;
	tt->height = texture_height;
	tt->tile_size = tile_size;
	
	// Build the 0 tile, a transparent gray checkerboard used for not yet allocated tiles
	size_t checker_size = 16;
	uint32_t pixel_data[tile_size][tile_size];
	
	// Start with black, loop over the checkers and draw each checker with the inner loops
	// Remember: integer values are little endian but colors are interpreted as big endian.
	// Therefore we write the colors as 0xAABBGGRR.
	uint32_t color = 0xc0a0a0a0;
	for(size_t cx = 0; cx < (tile_size / checker_size); cx++){
		for(size_t cy = 0; cy < (tile_size / checker_size); cy++){
			// Draw one checker
			for(size_t i = 0; i < checker_size; i++){
				for(size_t j = 0; j < checker_size; j++){
					pixel_data[cx * checker_size + i][cy * checker_size + j] = color;
				}
			}
			
			// Flip colors after each tile. Opaque black (0x000000ff) becomes transparent
			// white (0xffffff00) when each bit is flipped and vice versa.
			color = ~color;
		}
		
		// Flip once more after each row to get the checker pattern instead of stripes
		color = ~color;
	}
	
	// Create OpenGL texture, set its size and upload the checkerboard texture at tile 0
	glGenTextures(1, &tt->texture);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, tt->texture);
	glTexImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGBA, tt->width, tt->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexSubImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, 0, 0, tile_size, tile_size, GL_RGBA, GL_UNSIGNED_BYTE, pixel_data);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);
	
	return tt;
}

void tile_table_destroy(tile_table_p tile_table){
	// Now clean up the tile table resources
	glDeleteTextures(1, &tile_table->texture);
	free(tile_table);
}