#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "layer.h"
#include "common.h"
#include "math.h"
#include "viewport.h"

layer_p layers;
size_t layer_count;
GLuint layer_prog, layers_vertex_buffer;


void layers_load(){
	// Start with an empty layer list
	layers = NULL;
	layer_count = 0;
	
	layer_prog = load_and_link_program("tiles.vs", "tiles.ps");
	assert(layer_prog != 0);
	
	glGenBuffers(1, &layers_vertex_buffer);
	assert(layers_vertex_buffer != 0);
	
	if ( ! gl_ext_present("GL_ARB_texture_rectangle") ){
		fprintf(stderr, "OpenGL extention GL_ARB_texture_rectangle no supported!\n");
		exit(1);
	}
	glEnable(GL_TEXTURE_RECTANGLE_ARB);
}

void layers_unload(){
	free(layers);
	layers = NULL;
	layer_count = 0;
	
	glDeleteBuffers(1, &layers_vertex_buffer);
	glDisable(GL_TEXTURE_RECTANGLE_ARB);
}

void layer_new(int64_t x, int64_t y, int32_t z, uint64_t width, uint64_t height){
	layer_count++;
	layers = realloc(layers, layer_count * sizeof(layer_t));
	
	layer_p layer = &layers[layer_count-1];
	*layer = (layer_t){
		.x = x, .y = y, .z = z,
		.width = width,
		.height = height,
		.current_scale = NULL
	};
	
	// Sort layers in z order
	int sorter(const void *va, const void *vb){
		layer_p a = (layer_p)va;
		layer_p b = (layer_p)vb;
		if (a->z == b->z)
			return 0;
		return (a->z < b->z) ? -1 : 1;
	};
	qsort(layers, layer_count, sizeof(layer_t), sorter);
}

void layer_scale_new(layer_p layer, scale_index_t scale_index, tile_table_p tile_table, viewport_p viewport){
	// Calculate the number of required tile ids and allocate them
	float scale = vp_scale_for(viewport, scale_index);
	uint64_t width = ceilf(layer->width * scale);
	uint64_t height = ceilf(layer->height * scale);
	size_t tile_count = tile_table_tile_count_for_size(tile_table, width, height);
	layer_scale_p ls = calloc(1, LAYER_SCALE_HEADER_SIZE + tile_count * sizeof(tile_id_t));
	
	ls->scale_index = scale_index;
	ls->width = width;
	ls->height = height;
	ls->tile_count = tile_count;
	
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
}

void layer_scale_upload(layer_p layer, scale_index_t scale_index, const uint8_t *pixel_data, tile_table_p tile_table){
	// Find the layer scale
	layer_scale_p ls = layer->current_scale;
	if (ls->scale_index > scale_index) {
		while(ls != NULL && ls->scale_index > scale_index)
			ls = ls->smaller;
	} else {
		while(ls != NULL && ls->scale_index < scale_index)
			ls = ls->larger;
	}
	
	// Stop if we didn't find the matching layer
	if(ls == NULL || ls->scale_index != scale_index)
		return;
	
	// Allocate any missing tiles and upload the pixel data
	tile_table_alloc(tile_table, ls->tile_count, ls->tile_ids, ls);
	tile_table_upload(tile_table, ls->tile_count, ls->tile_ids, ls->width, ls->height, pixel_data);
}

/*

Image data upload workflow:

- Create shared mapping
- Send client draw request with id of shared mapping
	- Client maps shared memory
	- Client draws layer
	- Client unmaps shared memory
	- Client sends draw response
- Make sure that pages are allocated in the mega texture for this scale index
	- If no pages are yet allocated for the scale index do so
- Server copies pixel data to mega texture
	- Copy per page (easy)
	- Optimization: Find largest rectangles and copy per rectangle

*/


void layer_destroy(size_t index){
	// Not yet meaningfull
}


static void layer_scale_tile_id_to_offset(layer_scale_p layer_scale, tile_id_t id, tile_table_p tile_table, uint64_t *x, uint64_t *y){
	uint64_t tiles_per_line = iceildiv(layer_scale->width, tile_table->tile_size);
	*y = (id / tiles_per_line) * tile_table->tile_size;
	*x = (id % tiles_per_line) * tile_table->tile_size;
}

void layers_draw(viewport_p viewport, tile_table_p tile_table){
	// First allocate a vertex buffer for all tiles (not the perfect solution, we would actually need the count of displayed tiles)
	glBindBuffer(GL_ARRAY_BUFFER, layers_vertex_buffer);
	glBufferData(GL_ARRAY_BUFFER, tile_table->tile_count * 16 * sizeof(float), NULL, GL_STREAM_DRAW);
	float *buffer = glMapBuffer(GL_ARRAY_BUFFER, GL_READ_WRITE);
	
	// bo is short for buffer_offset
	size_t bo = 0;
	size_t t = tile_table->tile_size;
	size_t tiles_to_draw = 0;
	
	for(size_t i = 0; i < layer_count; i++){
		layer_p layer = &layers[i];
		layer_scale_p ls = layers[i].current_scale;
		float scale = vp_scale_for(viewport, ls->scale_index);
		
		// world space tile size (the smaller the scale the larger a tile becomes)
		size_t ws_t = t / scale;
		
		for(size_t j = 0; j < ls->tile_count; j++){
			uint64_t x, y;
			layer_scale_tile_id_to_offset(ls, j, tile_table, &x, &y);
			float ws_x = x / scale, ws_y = y / scale;
			
			uint32_t u, v;
			tile_table_id_to_offset(tile_table, ls->tile_ids[j], &u, &v);
			size_t w, h;
			w = (ls->width - x > t) ? t : ls->width - x;
			h = (ls->height - y > t) ? t : ls->height - y;
			float ws_w = w / scale, ws_h = h / scale;
			
			// Unfortunately we need all this casting stuff otherwise C will only
			// use unsigned values and break all negative coordinates.
			// TODO: adjust tiles size in world space to scale index
			buffer[bo++] = layer->x + ws_x +    0;  buffer[bo++] = layer->y + ws_y +    0;  buffer[bo++] = u + (float)0;  buffer[bo++] = v + (float)0;
			buffer[bo++] = layer->x + ws_x + ws_w;  buffer[bo++] = layer->y + ws_y +    0;  buffer[bo++] = u + (float)w;  buffer[bo++] = v + (float)0;
			buffer[bo++] = layer->x + ws_x + ws_w;  buffer[bo++] = layer->y + ws_y + ws_h;  buffer[bo++] = u + (float)w;  buffer[bo++] = v + (float)h;
			buffer[bo++] = layer->x + ws_x +    0;  buffer[bo++] = layer->y + ws_y + ws_h;  buffer[bo++] = u + (float)0;  buffer[bo++] = v + (float)h;
			
			tiles_to_draw++;
		}
	}
	/*
	printf("vertex buffer:\n");
	for(size_t i = 0; i < bo; i += 16){
		printf("  %f,%f (%.0f,%.0f)  %f,%f (%.0f,%.0f)\n  %f,%f (%.0f,%.0f)  %f,%f (%.0f,%.0f)\n--\n",
			buffer[i+0], buffer[i+1], buffer[i+2], buffer[i+3],
			buffer[i+4], buffer[i+5], buffer[i+6], buffer[i+7],
			buffer[i+8], buffer[i+9], buffer[i+10], buffer[i+11],
			buffer[i+12], buffer[i+13], buffer[i+14], buffer[i+15]
		);
	}
	*/
	
	glUnmapBuffer(GL_ARRAY_BUFFER);
	
	
	glUseProgram(layer_prog);
	
	GLint pos_tex_attrib = glGetAttribLocation(layer_prog, "pos_and_tex");
	assert(pos_tex_attrib != -1);
	glEnableVertexAttribArray(pos_tex_attrib);
	glVertexAttribPointer(pos_tex_attrib, 4, GL_FLOAT, GL_FALSE, sizeof(float) * 4, 0);
	
	GLint world_to_norm_uni = glGetUniformLocation(layer_prog, "world_to_norm");
	assert(world_to_norm_uni != -1);
	glUniformMatrix3fv(world_to_norm_uni, 1, GL_FALSE, viewport->world_to_normal);
	
	glActiveTexture(GL_TEXTURE0);
	GLint tex_attrib = glGetUniformLocation(layer_prog, "tex");
	assert(tex_attrib != -1);
	glUniform1i(tex_attrib, 0);
	
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, tile_table->texture);
	//glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	//glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glDrawArrays(GL_QUADS, 0, tiles_to_draw * 4);
	
	glBindTexture(GL_TEXTURE_2D, 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glUseProgram(0);
}