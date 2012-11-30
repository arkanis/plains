#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "layer.h"
#include "common.h"
#include "math.h"

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
	glDeleteBuffers(1, &layers_vertex_buffer);
	glDisable(GL_TEXTURE_RECTANGLE_ARB);
}

void layer_new(tile_table_p tile_table, int64_t x, int64_t y, int32_t z, uint64_t width, uint64_t height, const uint8_t *pixel_data){
	layer_count++;
	layers = realloc(layers, layer_count * sizeof(layer_t));
	
	layer_p layer = &layers[layer_count-1];
	*layer = (layer_t){
		.x = x, .y = y, .z = z,
		.width = width,
		.height = height
	};
	
	layer->tile_count = tile_table_tile_count_for_size(tile_table, width, height);
	layer->tile_ids = malloc(layer->tile_count * sizeof(tile_id_t));
	
	tile_table_alloc(tile_table, layer->tile_count, layer->tile_ids, layer);
	tile_table_upload(tile_table, layer->tile_count, layer->tile_ids, width, height, pixel_data);
	
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


static void layer_tile_id_to_offset(layer_p layer, tile_id_t id, tile_table_p tile_table, uint64_t *x, uint64_t *y){
	uint64_t tiles_per_line = iceildiv(layer->width, tile_table->tile_size);
	*y = (id / tiles_per_line) * tile_table->tile_size;
	*x = (id % tiles_per_line) * tile_table->tile_size;
}

void layers_draw(viewport_p viewport, tile_table_p tile_table){
	// First allocate a vertex buffer for all allocated tiles
	glBindBuffer(GL_ARRAY_BUFFER, layers_vertex_buffer);
	glBufferData(GL_ARRAY_BUFFER, tile_table->allocated_tiles * 16 * sizeof(float), NULL, GL_STREAM_DRAW);
	float *buffer = glMapBuffer(GL_ARRAY_BUFFER, GL_READ_WRITE);
	
	// bo is short for buffer_offset
	size_t bo = 0;
	size_t t = tile_table->tile_size;
	
	for(size_t i = 0; i < layer_count; i++){
		layer_p layer = &layers[i];
		
		for(size_t j = 0; j < layer->tile_count; j++){
			uint64_t x, y;
			layer_tile_id_to_offset(layer, j, tile_table, &x, &y);
			uint32_t u, v;
			tile_table_id_to_offset(tile_table, layer->tile_ids[j], &u, &v);
			size_t w, h;
			w = (layer->width - x > t) ? t : layer->width - x;
			h = (layer->height - y > t) ? t : layer->height - y;
			
			// Unfortunately we need all this casting stuff otherwise C will only
			// use unsigned values and break all negative coordinates.
			buffer[bo++] = layer->x + (int64_t)x + (int64_t)0;  buffer[bo++] = layer->y + (int64_t)y + (int64_t)0;  buffer[bo++] = u + (float)0 + 0.5;  buffer[bo++] = v + (float)0 + 0.5;
			buffer[bo++] = layer->x + (int64_t)x + (int64_t)w;  buffer[bo++] = layer->y + (int64_t)y + (int64_t)0;  buffer[bo++] = u + (float)w - 0.5;  buffer[bo++] = v + (float)0 + 0.5;
			buffer[bo++] = layer->x + (int64_t)x + (int64_t)w;  buffer[bo++] = layer->y + (int64_t)y + (int64_t)h;  buffer[bo++] = u + (float)w - 0.5;  buffer[bo++] = v + (float)h - 0.5;
			buffer[bo++] = layer->x + (int64_t)x + (int64_t)0;  buffer[bo++] = layer->y + (int64_t)y + (int64_t)h;  buffer[bo++] = u + (float)0 + 0.5;  buffer[bo++] = v + (float)h - 0.5;
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
	glDrawArrays(GL_QUADS, 0, tile_table->allocated_tiles * 4);
	
	glBindTexture(GL_TEXTURE_2D, 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glUseProgram(0);
}