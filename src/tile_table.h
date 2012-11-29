#pragma once

#include <stdint.h>


// Tile index type, used to index into the tile_table->tiles array
typedef uint32_t tile_id_t, *tile_id_p;


//
// Type for tile meta data kept on the CPU side. The actual pixel data it stored in the
// OpenGL texture of the tile table.
//
// The union is designed to be one pointer in size. Free tiles have an least significant bit of
// 1 and allocated tiles 0. Allocated tiles contain a pointer to a layer_scale_t structure which
// is at least 16 bit (2 byte) aligned. Therefore the least significant bit is always zero. Free
// tiles have the TILE_FREE flag set which sets the least significant bit to 1.
//

#define TILE_FREE 1<<0

typedef union {
	// When a tile is allocated it points to the layer scale that uses the tile
	void* used_by;
	// Data for free tiles (a basic free list)
	struct {
		// ID of the next free tile of 0 if no free tiles are left
		tile_id_t next_free_tile;
		// Flags for this tile
		uint32_t flags;
	};
} tile_t, *tile_p;


//
// Type to represent the entire tile table (texture, tile meta data, etc.)
//

typedef struct {
	// Side length (width and height) of one tile in pixels
	uint16_t tile_size;
	// Texture dimensions in pixels
	uint32_t width, height;
	// OpenGL texture object
	GLuint texture;
	
	// ID of the next free tile or 0 if no free tile is left
	tile_id_t next_free_tile;
	
	// Number of tiles the texture consists of
	size_t tile_count;
	tile_t* tiles;
} tile_table_t, *tile_table_p;


tile_table_p tile_table_new(uint32_t texture_width, uint32_t texture_height, uint16_t tile_size);
void tile_table_destroy(tile_table_p tile_table);

size_t tile_table_tile_count_for_size(tile_table_p tile_table, uint64_t width, uint64_t height);
void tile_table_alloc(tile_table_p tile_table, size_t tile_id_count, tile_id_p const tile_ids, void* used_by);
void tile_table_upload(tile_table_p tile_table, size_t tile_id_count, tile_id_t tile_ids[tile_id_count], uint64_t width, uint64_t height, const uint8_t *pixel_data);