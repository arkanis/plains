#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>

#include "tile_table.h"
#include "math.h"


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
	
	
	// Allocate and initialize page meta data structures
	tt->tile_count = (texture_width / tile_size) * (texture_height / tile_size);
	tt->tiles = malloc(tt->tile_count * sizeof(tile_t));
	tt->allocated_tiles = 0;
	
	// Initialize the page meta data with free tiles and a proper free list
	tt->tiles[tt->tile_count-1] = (tile_t){
		.next_free_tile = 0,
		.flags = TILE_FREE
	};
	for(ssize_t i = tt->tile_count - 2; i > -1; i--){
		tt->tiles[i] = (tile_t){
			.next_free_tile = i+1,
			.flags = TILE_FREE
		};
	}
	// Start with page 1 as the next free page (0 is reserved)
	tt->next_free_tile = 1;
	
	
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
	free(tile_table->tiles);
	free(tile_table);
}

void tile_table_alloc(tile_table_p tile_table, size_t tile_id_count, tile_id_p const tile_ids, void* used_by){
	for(size_t i = 0; i < tile_id_count; i++){
		tile_id_t free_tile = tile_table->next_free_tile;
		assert(free_tile != 0);
		tile_table->next_free_tile = tile_table->tiles[free_tile].next_free_tile;
		tile_table->tiles[free_tile].used_by = used_by;
		tile_ids[i] = free_tile;
	}
	
	tile_table->allocated_tiles += tile_id_count;
}


size_t tile_table_tile_count_for_size(tile_table_p tile_table, uint64_t width, uint64_t height){
	return iceildiv(width, tile_table->tile_size) * iceildiv(height, tile_table->tile_size);
}

void tile_table_id_to_offset(tile_table_p tile_table, tile_id_t id, uint32_t *x, uint32_t *y){
	uint32_t tiles_per_line = tile_table->width / tile_table->tile_size;
	*y = (id / tiles_per_line) * tile_table->tile_size;
	*x = (id % tiles_per_line) * tile_table->tile_size;
}

void tile_table_upload(tile_table_p tile_table, size_t tile_id_count, tile_id_t tile_ids[tile_id_count], uint64_t width, uint64_t height, const uint8_t *pixel_data){
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, tile_table->texture);
	glPixelStorei(GL_UNPACK_ROW_LENGTH, width);
	
	size_t tile_id_index = 0;
	for(size_t y = 0; y < height; y += tile_table->tile_size){
		for(size_t x = 0; x < width; x += tile_table->tile_size){
			if(tile_id_index >= tile_id_count)
				goto cleanup;
			
			uint32_t tile_x, tile_y;
			tile_table_id_to_offset(tile_table, tile_ids[tile_id_index], &tile_x, &tile_y);
			tile_id_index++;
			
			size_t tile_width = (width - x > tile_table->tile_size) ? tile_table->tile_size : width - x;
			size_t tile_height = (height - y > tile_table->tile_size) ? tile_table->tile_size : height - y;
			const uint8_t * tile_data = pixel_data + (y * width * 4) + x * 4;
			
			glTexSubImage2D(GL_TEXTURE_RECTANGLE_ARB, 0,
				tile_x, tile_y, tile_width, tile_height,
				GL_RGBA, GL_UNSIGNED_BYTE, tile_data);
		}
	}
	
	cleanup:
	glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);
}