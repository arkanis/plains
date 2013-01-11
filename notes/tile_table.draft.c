#include <stdbool.h>
#include <string.h>
#include <math.h>

#include "tile_table.h"
#include "viewport.h"


layer_p pixel_layer_new(int64_t x, int64_t y, uint64_t width, uint64_t height);
void layer_destroy(layer_p layer);

layer_scale_p layer_scale_new(layer_t layer, scale_index_t scale_index, tile_table_p tile_table);
void layer_scale_destroy(layer_scale_p layer_scale);
void layer_scale_upload(layer_scale_p layer_scale, uint64_t x, uint64_t y, uint64_t width, uint64_t height, void *pixeldata);


tile_table_p tile_table_new(uint32_t texture_width, uint32_t texture_height, uint16_t tile_size);
void tile_table_destroy(tile_table_p tile_table);
void tile_table_id_to_offset(tile_table_p tile_table, tile_id_t id, uint32_t *x, uint32_t *y);


/**
 * A small ceil function for integers.
 */
uint64_t iceildiv(uint64_t a, uint64_t b){
	uint64_t result = a / b;
	if (a % b != 0)
		result++;
	return result;
}




/**
 * Creates a new pixel layer. No layer scale or other data is allocated yet.
 */
layer_p pixel_layer_new(int64_t x, int64_t y, uint64_t width, uint64_t height){
	layer_p layer = malloc(sizeof(layer_p));
	layer->type = LAYER_PIXEL;
	layer->pixel.x = x;
	layer->pixel.y = y;
	layer->pixel.width = width;
	layer->pixel.height = height;
	layer->pixel.current_scale = NULL;
}

/**
 * Destroys the layer with all associated data (layer scales, etc.)
 */
void layer_destroy(layer_p layer){
	switch(layer->type){
		case LAYER_PIXEL:
			// Free all associated layer scales. The layer_scale_destroy() function updates
			// our current_scale pointer to the next layer scale. Therefore we loop until no
			// further scales are left.
			while(layer->current_scale != NULL)
				layer_scale_destroy(layer->current_scale);
			break;
		case LAYER_VERTEX:
			assert(0);
			break;
	}

	free(layer);
}


/**
 * Creates a new layer scale for the specified layer and associates it with the specified
 * tile table. All tile ids are left at 0.
 * 
 * Maintains the scale linked list of the layer by inserting the layer scale at the right
 * position. Also takes care of the recently used list of the tile table by inserting the
 * layer scale at the front of it.
 */
layer_scale_p layer_scale_new(layer_t layer, scale_index_t scale_index, tile_table_p tile_table, viewport_t viewport){
	assert(layer->type == LAYER_PIXEL);
	
	// Calculate the number of required tile ids and allocate them
	float scale = vp_scale_for(viewport, scale_index);
	uint64_t width = ceilf(layer->pixel.width * scale);
	uint64_t height = ceilf(layer->pixel.height * scale);
	size_t tile_id_count = iceildiv(width, tile_table->tile_size) * iceildiv(height, tile_table->tile_size);
	layer_scale_p ls = calloc(1, LAYER_SCALE_HEADER_SIZE + tile_id_count * sizeof(tile_id_t));
	
	ls->tile_id_count = tile_id_count;
	ls->scale_index = scale_index;
	ls->layer = layer;
	ls->width = width;
	ls->height = height;
	
	// Maintain the layers scale list
	layer_scale_p cls = layer->current_scale;
	if (cls == NULL) {
		// Layer has no layer scales whatsoever
		layer->current_scale = ls;
		ls->larger = NULL;
		ls->smaller = NULL;
	} else if (scale_index < cls->scale_index) {
		// Insert a smaller layer scale
		while(cls->smaller != NULL && scale_index < cls->smaller->scale_index)
			cls = cls->smaller;
		// cls is now the next largest scale to us
		ls->smaller = cls->smaller;
		ls->larger = cls;
		if (cls->smaller != NULL)
			cls->smaller->larger = ls;
		cls->smaller = ls;
	} else {
		// Insert a larger layer scale
		while(cls->larger != NULL && scale_index > cls->larger->scale_index)
			cls = cls->larger;
		// cls is now the next smallest scale to us
		ls->smaller = cls;
		ls->larger = cls->larger;
		if (cls->larger != NULL)
			cls->larger->smaller = ls;
		cls->larger = ls;
	}
	
	// Maintain the tile tables recently used list by adding this layer scale in front
	// of it as the most recently used one.
	ls->tile_table = tile_table;
	ls->more_used = NULL;
	ls->less_used = tile_table->most_used;
	if (tile_table->most_used)
		tile_table->most_used->more_used = ls;
	tile_table->most_used = ls;
	if (tile_table->least_used == NULL)
		tile_table->least_used = ls;
}

/**
 * Destroys the layer and frees all associated resources (memory as well as tiles).
 * 
 * We also maintain the scale index and recently used linked lists. This involves updating
 * the layers current_scale pointer and the tile tables most_used and least_used pointers
 * if necessary.
 */
void layer_scale_destroy(layer_scale_p layer_scale){
	assert(layer_scale->layer->type == LAYER_PIXEL);
	
	// Maintain the scale index list
	if (layer_scale->larger != NULL)
		layer_scale->larger->smaller = layer_scale->smaller;
	if (layer_scale->smaller != NULL)
		layer_scale->smaller->larger = layer_scale->larger;
	
	// Update the layers current_scale pointer if necessary
	if (layer_scale->layer->pixel.current_scale == layer_scale){
		if (layer_scale->larger != NULL)
			layer_scale->layer->pixel.current_scale = layer_scale->larger
		else
			layer_scale->layer->pixel.current_scale = layer_scale->smaller;
	}
	
	// Maintain the recently used list
	if (layer_scale->more_used != NULL)
		layer_scale->more_used->less_used = layer_scale->less_used;
	if (layer_scale->less_used != NULL)
		layer_scale->less_used->more_used = layer_scale->more_used;
	
	// Update the tile tables recently used list pointers if necessary
	if (layer_scale->tile_table->most_used == layer_scale)
		layer_scale->tile_table->most_used = layer_scale->less_used;
	if (layer_scale->tile_table->least_used == layer_scale)
		layer_scale->tile_table->least_used = layer_scale->more_used;
	
	free(layer_scale);
}

/**
 * 
 */
void layer_scale_upload(layer_scale_p layer_scale, uint64_t x, uint64_t y, uint64_t width, uint64_t height, void *pixeldata){
	tile_table_p tt = layer_scale->tile_table;
	assert(x % tt->tile_size == 0 && y % tt->tile_size == 0 && width % tt->tile_size == 0 && height % tt->tile_size == 0);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, tt->texture);
	
	// Iterate over all tiles of the region
	for(size_t region_tx = 0; region_tx < width / tt->tile_size; region_tx++){
		for(size_t region_ty = 0; region_ty < height / tt->tile_size; region_ty++){
			size_t scale_tx = (x / tt->tile_size) + region_tx;
			size_t scale_ty = (y / tt->tile_size) + region_ty;
			size_t scale_tiles_per_line = layer_scale->width / tt->tile_size;
			size_t scale_tile_id_index = scale_ty * scale_tiles_per_line + scale_tx;
			
			// We need all tiles allocated, so allocate any missing tiles
			if (layer_scale->tile_ids[scale_tile_id_index] == 0){
				tile_id_t free_tile = layer_scale->tile_table->next_free_tile;
				assert(free_tile != 0);
				layer_scale->tile_ids[scale_tile_id_index] = free_tile;
				layer_scale->tile_table->next_free_tile = layer_scale->tile_table->tiles[free_tile].next_free_tile;
				layer_scale->tile_table->tiles[free_tile].used_by = layer_scale;
			}
			
			// Upload data to texture
			uint32_t tex_x, tex_y;
			tile_table_id_to_offset(layer_scale->tile_table, layer_scale->tile_ids[tile_id_index], &tex_x, &tex_y);
			glTexSubImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, tex_x, tex_y, layer_scale->tile_table->tile_size, layer_scale->tile_table->tile_size, GL_RGBA, GL_UNSIGNED_BYTE, pixel_data);
		}
	}
	
	size_t tiles_per_line = layer_scale->width / tt->tile_size;
	for(size_t tx = x / tt->tile_size; tx <= (x+width) / tt->tile_size; tx++){
		for(size_t ty = y / tt->tile_size; ty <= (y+height) / tt->tile_size; ty++){
			size_t tile_id_index = ty * tiles_per_line + tx;
			
			if (layer_scale->tile_ids[tile_id_index] == 0){
				// Tile not allocated but we need it. So allocate it.
				tile_id_t free_tile = layer_scale->tile_table->next_free_tile;
				assert(free_tile != 0);
				layer_scale->tile_ids[tile_id_index] = free_tile;
				layer_scale->tile_table->next_free_tile = layer_scale->tile_table->tiles[free_tile].next_free_tile;
				layer_scale->tile_table->tiles[free_tile].used_by = layer_scale;
			}
			
			// Upload data to texture
			uint32_t tex_x, tex_y;
			tile_table_id_to_offset(layer_scale->tile_table, layer_scale->tile_ids[tile_id_index], &tex_x, &tex_y);
			glTexSubImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, x, y, layer_scale->tile_table->tile_size, layer_scale->tile_table->tile_size, GL_RGBA, GL_UNSIGNED_BYTE, pixel_data);
		}
	}
	
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);
}

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
	
	// Start with an empty used list of layer scales
	tt->most_used = NULL;
	tt->least_used = NULL;
	
	// Allocate and initialize page meta data structures
	tt->tile_count = (texture_width / tile_size) * (texture_height / tile_size);
	tt->tiles = malloc(tt->tile_count * sizeof(tile_t));
	
	// Initialize the page meta data with free tiles and a proper free list
	tt->tiles[tt->tile_count-1] = (tile_t){
		.next_free_page = 0,
		.flags = TILE_FREE
	};
	for(ssize_t i = tt->tile_count - 2; i > -1; i--){
		tt->tiles[i] = (tile_t){
			.next_free_page = i+1,
			.flags = TILE_FREE
		};
	}
	// Start with page 1 as the next free page (0 is reserved)
	tt->next_free_page = 1;
	
	// Build the 0 tile, a transparent gray checkerboard used for not yet allocated tiles
	size_t checker_size = 16;
	uint32_t pixel_data[tile_size][tile_size];
	
	// Start with black, loop over the checkers and draw each checker with the inner loops
	uint32_t color = 0x000000ff;
	for(size_t cx = 0; cx < tile_size / checker_size; cx++){
		for(size_t cy = 0; cy < tile_size / checker_size; cy++){
			// Draw one checker
			for(size_t i = 0; i < tile_size; i++)
				for(size_t j = 0; j < tile_size; j++)
					pixel_data[cx * checker_size + i][cy * checker_size + j] = color;
			
			// Flip colors after each tile. Opaque black (0x000000ff) becomes transparent
			// white (0xffffff00) when each bit is flipped and vice versa.
			color = ~color;
		}
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
	// First free all layer scales (that in turn frees the tiles)
	while(tile_table->most_used != NULL)
		layer_scale_destroy(tile_table->most_used);
	
	// Now clean up the tile table resources
	glDeleteTextures(1, &tile_table->texture);
	free(tile_table->tiles);
	free(tile_table);
}

void tile_table_id_to_offset(tile_table_p tile_table, tile_id_t id, uint32_t *x, uint32_t *y){
	uint32_t tiles_per_line = tile_table->width / tile_table->tile_size;
	*y = (id / tiles_per_line) * tile_table->tile_size;
	*x = (id % tiles_per_line) * tile_table->tile_size;
}










page_segment_p pt_alloc(page_table_p pt, uint32_t width, uint32_t height){
	// Calculate how many pages we need
	uint32_t uprounded_div(uint32_t a, uint32_t b){
		uint32_t result = a / b;
		if (a % b > 0)
			result++;
		return result;
	}
	
	uint32_t page_count = uprounded_div(width, pt->page_size) * uprounded_div(height, pt->page_size);
	
	// Allocate segment
	page_segment_p segment = malloc(PAGE_SEGMENT_HEADER_SIZE + page_count * sizeof(page_idx_t));
	segment->page_count = page_count;
	segment->flags = 0;
	
	// Allocate individual pages
	for(size_t i = 0; i < page_count; i++){
		if (pt->next_free_page == PAGE_IDX_NONE) {
			// No free pages left, reclaim the least used segment
			page_segment_p seg = remove_least_used_segment(pt);
			pt_free(seg);
		}
		
		// We still got free pages available
		page_idx_t page_idx = pt->next_free_page;
		assert(pt->pages[page_idx].flags & PAGE_FREE);
		segment->pages[i] = page_idx;
		pt->next_free_page = pt->pages[page_idx].next_free_page;
		pt->pages[page_idx].segment = segment;
	}
	
	// Insert segment into recently used list. We insert it at the front of the list. If we would add it at the end it might be
	// reclaimed when the next segment is allocated. If an entire draw operation requires multiple segments this might
	// cause an endless loop as segments are invalidated and allocated again and again.
	add_as_newest_segment(pt, segment);
}

void pt_free(page_table_p pt, page_segment_p segment){
	segment->flags |= PAGE_SEGMENT_INVALID;
}

void pt_mark_segment_used(page_table_p pt, page_segment_p segment){
	// ToDo
}

static page_segment_p remove_least_used_segment(page_table_p pt){
	page_segment_p segment = pt->least_used;
	pt->least_used = segment->prev;
	if (pt->most_used == segment)
		pt->most_used = NULL;
	return segment;
}

static void add_as_newest_segment(page_table_p pt, page_segment_p segment){
	segment->prev = NULL;
	segment->next = pt->most_used;
	pt->most_used = segment;
	if (pt->least_used == NULL)
		pt->least_used = segment;
}