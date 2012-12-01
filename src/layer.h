#pragma once
#include <stdint.h>
#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>

#include "viewport.h"
#include "tile_table.h"



// The scale index represents how far the user zoomed in or out
typedef int8_t scale_index_t;


//
// A pixel layer consits of zero or more layer scales. These contain the pixel data of the
// layer for a specific scale index. The pixel data itself is stored as tiles in the tile
// table texture. The layer scale contains the ids of the tiles that contain the pixel data.
//

typedef struct layer_scale_s layer_scale_t, *layer_scale_p;
struct layer_scale_s {
	// Scale index of this layer scale
	scale_index_t scale_index;
	
	// Dimensions of this layer scale. We could calculate them out of the scale index but that
	// would required to ask the viewport to translate the scale index to a scale. To avoid the
	// extra dependency to the viewport we store them here.
	uint64_t width, height;
	
	// Double linked list of scales for this layer ordered by scale index
	layer_scale_p larger, smaller;
	
	// Tile ids for this scale
	size_t tile_count;
	tile_id_t tile_ids[];
};

#define LAYER_SCALE_HEADER_SIZE (offsetof(struct layer_scale_s, tile_ids))


//
// A layer is a drawable object of pixel data rendered in multiple scales
//

typedef struct {
	// Position of the upper left corner and dimensions of the layer for scale
	// index 0 (original size)
	int64_t x, y;
	uint64_t width, height;
	int32_t z;
	
	// Pointer into a double linked list of layer scales. We just remember the
	// pointer to the current scale. If we zoom in we follow the larger link,
	// when zooming out we follow the smaller link.
	layer_scale_p current_scale;
} layer_t, *layer_p;

extern layer_p layers;
extern size_t layer_count;


void layers_load();
void layers_unload();
void layer_new(int64_t x, int64_t y, int32_t z, uint64_t width, uint64_t height);
void layer_scale_new(layer_p layer, scale_index_t scale_index, tile_table_p tile_table, viewport_p viewport);
void layer_scale_upload(layer_p layer, scale_index_t scale_index, const uint8_t *pixel_data, tile_table_p tile_table);
void layer_destroy(size_t index);
void layers_draw(viewport_p viewport, tile_table_p tile_table);