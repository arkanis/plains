#pragma once
#include <stdint.h>
#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>

#include "viewport.h"
#include "tile_table.h"


typedef struct {
	int64_t x, y;
	uint64_t width, height;
	int32_t z;
	
	size_t tile_count;
	tile_id_p tile_ids;
} layer_t, *layer_p;

extern layer_p layers;
extern size_t layer_count;


void layers_load();
void layers_unload();
void layer_new(tile_table_p tile_table, int64_t x, int64_t y, int32_t z, uint64_t width, uint64_t height, const uint8_t *pixel_data);
void layer_destroy(size_t index);
void layers_draw(viewport_p viewport, tile_table_p tile_table);