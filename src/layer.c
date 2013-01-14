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

draw_request_tree_p layers_in_rect(int64_t screen_x, int64_t screen_y, uint64_t screen_width, uint64_t screen_height, float current_scale_exp){
	draw_request_tree_p tree = draw_request_tree_new((draw_request_t){});
	
	for(size_t i = 0; i < layer_count; i++){
		layer_p layer = &layers[i];
		
		inline int64_t min(int64_t a, int64_t b){ return a < b ? a : b; }
		inline int64_t max(int64_t a, int64_t b){ return a > b ? a : b; }
		
		// Use signed ints for all values so C does not get funny (unsigned) ideas in any calculations
		int64_t lx1 = layer->x, ly1 = layer->y, sx1 = screen_x, sy1 = screen_y;
		int64_t lx2 = layer->x + layer->width, ly2 = layer->y + layer->height, sx2 = screen_x + screen_width, sy2 = screen_y + screen_height;
		
		printf("lx1 %ld, ly1 %ld, lx2 %ld, ly2 %ld\n", lx1, ly1, lx2, ly2);
		printf("sx1 %ld, sy1 %ld, sx2 %ld, sy2 %ld\n", sx1, sy1, sx2, sy2);
		int64_t rx1 = max(lx1, sx1), rx2 = min(lx2, sx2);
		int64_t ry1 = max(ly1, sy1), ry2 = min(ly2, sy2);
		printf("rx1 %ld, ry1 %ld, rx2 %ld, ry2 %ld\n", rx1, ry1, rx2, ry2);
		int64_t wx = rx2 - rx1;
		int64_t wy = ry2 - ry1;
		
		printf("wx %ld, wy %ld\n", wx, wy);
		if (wx < 0 || wy < 0)
			break;
		
		printf("world_x %ld, world_y %ld, object_x %ld, object_y %ld\n", rx1, ry1, rx1 - lx1, ry1 - ly1);
		draw_request_tree_append(tree, (draw_request_t){
			.world_x = rx1, .world_y = ry1,
			.object_x = rx1 - lx1, .object_y = ry1 - ly1,
			.width = wx,
			.height = wy,
			.scale_exp = (layer->current_scale) ? layer->current_scale->scale_index : current_scale_exp,
			.shm_fd = -1, .req_seq = 0, .flags = 0
		});
		
		/*
		int64_t offset_x = layer->x - screen_x;
		int64_t offset_y = layer->y - screen_y;
		
		if ( !( offset_x > -layer->width && offset_x < screen_width && offset_y > -layer->height && offset_y < screen_height ) )
			break;
		
		
		draw_request_tree_append(tree, (draw_request_t){
			.screen_x = max(offset_x, 0),  .screen_y = max(offset_y, 0),
			.object_x = max(-offset_x, 0), .object_y = max(-offset_y, 0),
			.width = min(layer->x + layer->width, screen_x + screen_width) - max(layer->x, screen_x),
			.height = min(layer->y + layer->height, screen_y + screen_height) - max(layer->y, screen_y),
			.scale_exp = (layer->current_scale) ? layer->current_scale->scale_index : current_scale_exp,
			.shm_fd = -1, .req_seq = 0, .flags = 0
		});
		*/
		/*
		// Use signed ints for all values so C does not get funny (unsigned) ideas in any calculations
		int64_t lx = layer->x, ly = layer->y, sx = screen_x, sy = screen_y;
		int64_t lw = layer->width, lh = layer->height, sw = screen_width, sh = screen_height;
		
		if (lx > sx + sw || lx + lw < sx || ly > sy + sh || ly + lh < sy)
			break;
		
		inline int64_t min(int64_t a, int64_t b){ return a < b ? a : b; }
		inline int64_t max(int64_t a, int64_t b){ return a > b ? a : b; }
		
		int64_t wx = max(lx, sx), wy = max(ly, sy);
		int64_t wx2 = min(lx+lw, sx+sw), wy2 = min(ly+lh, sy+sh);
		
		draw_request_tree_append(tree, (draw_request_t){
			.screen_x = wx - sx, .screen_y = wy - sy,
			.object_x = wx - lx, .object_y = wy - ly,
			.width = wx2 - wx, .height = wy2 - wy,
			.layer_idx = i,
			.scale_exp = (layer->current_scale) ? layer->current_scale->scale_index : current_scale_exp,
			.shm_fd = -1, .req_seq = 0, .flags = 0
		});
		*/
	}
	
	return tree;
}


size_t layer_new(int64_t x, int64_t y, int32_t z, uint64_t width, uint64_t height, void* private){
	layer_count++;
	layers = realloc(layers, layer_count * sizeof(layer_t));
	
	layer_p layer = &layers[layer_count-1];
	*layer = (layer_t){
		.x = x, .y = y, .z = z,
		.width = width,
		.height = height,
		.private = private,
		.current_scale = NULL
	};
	
	// Sort layers in z order
	/* Not for now since we use the index as ID
	int sorter(const void *va, const void *vb){
		layer_p a = (layer_p)va;
		layer_p b = (layer_p)vb;
		if (a->z == b->z)
			return 0;
		return (a->z < b->z) ? -1 : 1;
	};
	qsort(layers, layer_count, sizeof(layer_t), sorter);
	*/
	
	return layer_count-1;
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

/*
void layer_scale_upload(layer_p layer, scale_index_t scale_index, uint64_t x, uint64_t x, uint64_t width, uint64_t height, const uint8_t *pixel_data, tile_table_p tile_table){
	// Find the layer scale
	layer_scale_p ls = layer->current_scale;
	if (ls->scale_index > scale_index) {
		while(ls != NULL && ls->scale_index > scale_index)
			ls = ls->smaller;
	} else {
		while(ls != NULL && ls->scale_index < scale_index)
			ls = ls->larger;
	}
	
	// Stop if we didn't find the matching layer scale
	if(ls == NULL || ls->scale_index != scale_index)
		return;
	
	// Allocate any missing tiles and upload the pixel data
	tile_table_alloc(tile_table, ls->tile_count, ls->tile_ids, ls);
	tile_table_upload(tile_table, ls->tile_count, ls->tile_ids, ls->width, ls->height, pixel_data);
}
*/

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

/*
static void layer_scale_tile_id_to_offset(layer_scale_p layer_scale, tile_id_t id, tile_table_p tile_table, uint64_t *x, uint64_t *y){
	uint64_t tiles_per_line = iceildiv(layer_scale->width, tile_table->tile_size);
	*y = (id / tiles_per_line) * tile_table->tile_size;
	*x = (id % tiles_per_line) * tile_table->tile_size;
}
*/
/*
void layers_draw(viewport_p viewport, tile_table_p tile_table, layer_draw_proc_t draw_proc){
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
		// Ignore all layers that do not yet have a scale
		if (ls == NULL)
			continue;
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
			buffer[bo++] = layer->x + ws_x +    0;  buffer[bo++] = layer->y + ws_y +    0;  buffer[bo++] = u + (float)0;  buffer[bo++] = v + (float)0;
			buffer[bo++] = layer->x + ws_x + ws_w;  buffer[bo++] = layer->y + ws_y +    0;  buffer[bo++] = u + (float)w;  buffer[bo++] = v + (float)0;
			buffer[bo++] = layer->x + ws_x + ws_w;  buffer[bo++] = layer->y + ws_y + ws_h;  buffer[bo++] = u + (float)w;  buffer[bo++] = v + (float)h;
			buffer[bo++] = layer->x + ws_x +    0;  buffer[bo++] = layer->y + ws_y + ws_h;  buffer[bo++] = u + (float)0;  buffer[bo++] = v + (float)h;
			
			tiles_to_draw++;
			// Reset the tile age since we'll use it in this draw operation
			tile_table->tile_ages[ls->tile_ids[j]] = 0;
		}
	}
	
	printf("vertex buffer:\n");
	for(size_t i = 0; i < bo; i += 16){
		printf("  %f,%f (%.0f,%.0f)  %f,%f (%.0f,%.0f)\n  %f,%f (%.0f,%.0f)  %f,%f (%.0f,%.0f)\n--\n",
			buffer[i+0], buffer[i+1], buffer[i+2], buffer[i+3],
			buffer[i+4], buffer[i+5], buffer[i+6], buffer[i+7],
			buffer[i+8], buffer[i+9], buffer[i+10], buffer[i+11],
			buffer[i+12], buffer[i+13], buffer[i+14], buffer[i+15]
		);
	}
	
	
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
	
	tile_table_cycle(tile_table);
}
*/

/*
size_t layer_scale_tile_count_for_rect(layer_scale_p, tile_table_p tile_table, uint64_t x, uint64_t x, uint64_t width, uint64_t height){
	// Calculate affected size (the size of the rect when x and y are aligned to their tile boundaries)
	uint64_t affected_width = width + (x % tile_table->tile_size);
	uint64_t affected_height = height + (y % tile_table->tile_size);
	
	return iceildiv(affected_width, tile_table->tile_size) * iceildiv(affected_height, tile_table->tile_size);
}

void layer_scale_tile_ids_for_rect(layer_scale_p, tile_table_p tile_table, uint64_t x, uint64_t x, uint64_t width, uint64_t height, tile_id_p tiles){
	// Calculate affected size (the size of the rect when x and y are aligned to their tile boundaries)
	uint64_t affected_width = width + (x % tile_table->tile_size);
	uint64_t affected_height = height + (y % tile_table->tile_size);
	size_t tile_count_x = affected_width / tile_table->tile_size;
	size_t tile_count_y = affected_height / tile_table->tile_size;
	size_t tile_start_x = x / tile_table->tile_size;
	size_t tile_start_y = y / tile_table->tile_size;
	
	for(size_t j = 0; j < tile_count_y; j++){
		for(size_t i = 0; i < tile_count_x; i++){
			// Unfinished...
			tiles[j * tile_count_x + i] = 
		}
	}
}
*/