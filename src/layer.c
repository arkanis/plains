#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "layer.h"
#include "common.h"

layer_p layers;
size_t layer_count;
GLuint layer_prog, layers_vertex_buffer;


void layers_load(){
	// Start with an empty layer list
	layers = NULL;
	layer_count = 0;
	
	layer_prog = load_and_link_program("layer.vs", "layer.ps");
	assert(layer_prog != 0);
	
	glGenBuffers(1, &layers_vertex_buffer);
	assert(layers_vertex_buffer != 0);
	
	glBindBuffer(GL_ARRAY_BUFFER, layers_vertex_buffer);
	const float vertecies[] = {
		// Rectangle
		0, 0,
		1, 0,
		1, 1,
		0, 1
	};
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertecies), vertecies, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	
	if ( ! gl_ext_present("GL_ARB_texture_rectangle") ){
		fprintf(stderr, "OpenGL extention GL_ARB_texture_rectangle no supported!\n");
		exit(1);
	}
	glEnable(GL_TEXTURE_RECTANGLE_ARB);
}

void layers_unload(){
	for(size_t i = 0; i < layer_count; i++)
		glDeleteTextures(1, &layers[i].texture);
	glDisable(GL_TEXTURE_RECTANGLE_ARB);
}

void layer_new(int64_t x, int64_t y, int32_t z, uint64_t width, uint64_t height, const uint8_t *pixel_data){
	layer_count++;
	layers = realloc(layers, layer_count * sizeof(layer_t));
	
	layers[layer_count-1] = (layer_t){
		.x = x, .y = y, .z = z,
		.width = width,
		.height = height,
		.texture = 0
	};
	glGenTextures(1, &layers[layer_count-1].texture);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, layers[layer_count-1].texture);
	glTexImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixel_data);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);
	
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

void layer_destroy(size_t index){
	// Not yet meaningfull
}

void layers_draw(viewport_p viewport){
	glUseProgram(layer_prog);
	glBindBuffer(GL_ARRAY_BUFFER, layers_vertex_buffer);
	
	GLint pos_attrib = glGetAttribLocation(layer_prog, "pos");
	assert(pos_attrib != -1);
	glEnableVertexAttribArray(pos_attrib);
	glVertexAttribPointer(pos_attrib, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, 0);
	
	GLint world_to_norm_uni = glGetUniformLocation(layer_prog, "world_to_norm");
	assert(world_to_norm_uni != -1);
	glUniformMatrix3fv(world_to_norm_uni, 1, GL_FALSE, viewport->world_to_normal);
	
	glActiveTexture(GL_TEXTURE0);
	GLint tex_attrib = glGetUniformLocation(layer_prog, "tex");
	assert(tex_attrib != -1);
	glUniform1i(tex_attrib, 0);
	
	for(size_t i = 0; i < layer_count; i++){
		layer_p layer = &layers[i];
		
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB, layer->texture);
		
		GLint pos_and_size_uni = glGetUniformLocation(layer_prog, "pos_and_size");
		assert(pos_and_size_uni != -1);
		glUniform4f(pos_and_size_uni, layer->x, layer->y, layer->width, layer->height);
		
		glDrawArrays(GL_QUADS, 0, 4);
	}
	
	glBindTexture(GL_TEXTURE_2D, 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glUseProgram(0);
}