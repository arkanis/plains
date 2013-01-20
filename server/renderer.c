#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>

#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <errno.h>

#include "renderer.h"


static bool gl_ext_present(const char *ext_name);
static GLuint create_and_compile_shader(GLenum shader_type, const char *filename);
static GLuint load_and_link_program(const char *vertex_shader_filename, const char *fragment_shader_filename);
static void delete_program_and_shaders(GLuint program);


renderer_p renderer_new(uint16_t window_width, uint16_t window_height, const char *title){
	renderer_p renderer = malloc(sizeof(renderer_t));
	*renderer = (renderer_t){
		.texture = 0,
		.prog = 0,
		.vertex_buffer = 0
	};
	
	// Create window
	SDL_Init(SDL_INIT_VIDEO);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_WM_SetCaption(title, NULL);
	
	// Initialize SDL video mode, OpenGL and create render texture
	renderer_resize(renderer, window_width, window_height);
	
	// Enable alpha blending, create vertex buffer and load shaders
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	
	glEnable(GL_LINE_SMOOTH);
	glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
	
	renderer->prog = load_and_link_program("shader/tiles.vs", "shader/tiles.ps");
	assert(renderer->prog != 0);
	
	glGenBuffers(1, &renderer->vertex_buffer);
	assert(renderer->vertex_buffer != 0);
	
	return renderer;
}

void renderer_destroy(renderer_p renderer){
	free(renderer);
	SDL_Quit();
}

void renderer_resize(renderer_p renderer, uint16_t window_width, uint16_t window_height){
	SDL_SetVideoMode(window_width, window_height, 24, SDL_OPENGL | SDL_RESIZABLE);
	glViewport(0, 0, window_width, window_height);
	
	if ( !gl_ext_present("GL_ARB_texture_rectangle") ){
		fprintf(stderr, "OpenGL extention GL_ARB_texture_rectangle required!\n");
		return;
	}
	glEnable(GL_TEXTURE_RECTANGLE_ARB);
	
	// Free old buffer texture if there is one
	if (renderer->texture != 0)
		glDeleteTextures(1, &renderer->texture);
	
	// Create new one for the current resolution
	glGenTextures(1, &renderer->texture);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, renderer->texture);
	glTexImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGBA, window_width, window_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);
}


void renderer_clear(renderer_p renderer){
	glClearColor(1, 1, 1, 1.0);
	glClear(GL_COLOR_BUFFER_BIT);
}

void renderer_draw_response(renderer_p renderer, draw_request_p req){
	// Make sure we have the pixel data
	assert( (req->flags & DRAW_REQUEST_BUFFERED) && req->shm_fd != -1 );
	
	// Map shared memory and upload the pixel data. When done unmap the
	// shared memory.
	size_t shm_size = req->bw * req->bh * 4;
	void *pixel_data = mmap(NULL, shm_size, PROT_READ, MAP_SHARED, req->shm_fd, 0);
	assert(pixel_data != NULL);
	
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, renderer->texture);
	glTexSubImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, 0, 0, req->bw, req->bh, GL_RGBA, GL_UNSIGNED_BYTE, pixel_data);
	//glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);
	
	munmap(pixel_data, shm_size);
	
	// Allocate vertex buffer object for drawing
	glBindBuffer(GL_ARRAY_BUFFER, renderer->vertex_buffer);
	glBufferData(GL_ARRAY_BUFFER, 16 * sizeof(float), NULL, GL_STREAM_DRAW);
	float *buffer = glMapBuffer(GL_ARRAY_BUFFER, GL_READ_WRITE);
	
	// bo is short for buffer_offset
	size_t bo = 0;
	int64_t x = req->wx, y = req->wy, w = req->ww, h = req->wh;
	buffer[bo++] = x + 0;  buffer[bo++] = y + 0;  buffer[bo++] =       0;  buffer[bo++] =       0;
	buffer[bo++] = x + w;  buffer[bo++] = y + 0;  buffer[bo++] = req->bw;  buffer[bo++] =       0;
	buffer[bo++] = x + w;  buffer[bo++] = y + h;  buffer[bo++] = req->bw;  buffer[bo++] = req->bh;
	buffer[bo++] = x + 0;  buffer[bo++] = y + h;  buffer[bo++] =       0;  buffer[bo++] = req->bh;
	
	glUnmapBuffer(GL_ARRAY_BUFFER);
	
	
	glUseProgram(renderer->prog);
	
	GLint pos_tex_attrib = glGetAttribLocation(renderer->prog, "pos_and_tex");
	assert(pos_tex_attrib != -1);
	glEnableVertexAttribArray(pos_tex_attrib);
	glVertexAttribPointer(pos_tex_attrib, 4, GL_FLOAT, GL_FALSE, sizeof(float) * 4, 0);
	
	GLint world_to_norm_uni = glGetUniformLocation(renderer->prog, "world_to_norm");
	assert(world_to_norm_uni != -1);
	glUniformMatrix3fv(world_to_norm_uni, 1, GL_FALSE, req->transform);
	
	GLint color_uni = glGetUniformLocation(renderer->prog, "color");
	if (color_uni != -1)
		glUniform4f(color_uni, req->color.r, req->color.g, req->color.b, req->color.a);
	
	glActiveTexture(GL_TEXTURE0);
	GLint tex_attrib = glGetUniformLocation(renderer->prog, "tex");
	if(tex_attrib != -1)
		glUniform1i(tex_attrib, 0);
	
	//glBindTexture(GL_TEXTURE_RECTANGLE_ARB, renderer->texture);
	//glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	//glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	//glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glDrawArrays(GL_QUADS, 0, 4);
	
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glUseProgram(0);
}

void render_finish_draw(renderer_p renderer){
	SDL_GL_SwapBuffers();
}


//
// Support stuff
//

/**
 * Loads and compiles a source code file as a shader.
 * 
 * Returns the shaders GL object id on success or 0 on error.
 */
static GLuint create_and_compile_shader(GLenum shader_type, const char *filename){
	int fd = open(filename, O_RDONLY, 0);
	if (fd == -1){
		fprintf(stderr, "failed to load shader %s: %s\n", filename, sys_errlist[errno]);
		return 0;
	}
	struct stat file_stat;
	fstat(fd, &file_stat);
	char *code = mmap(NULL, file_stat.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
	
	GLuint shader = glCreateShader(shader_type);
	glShaderSource(shader, 1, (const char*[]){ code }, (const int[]){ file_stat.st_size });
	
	munmap(code, file_stat.st_size);
	close(fd);
	
	glCompileShader(shader);
	
	GLint result = GL_TRUE;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &result);
	if (result == GL_FALSE){
		char buffer[1024];
		glGetShaderInfoLog(shader, 1024, NULL, buffer);
		printf("shader compilation of %s failed:\n%s\n", filename, buffer);
		return 0;
	}
	
	return shader;
}

static GLuint load_and_link_program(const char *vertex_shader_filename, const char *fragment_shader_filename){
	// Load shaders
	GLuint vertex_shader = create_and_compile_shader(GL_VERTEX_SHADER, vertex_shader_filename);
	GLuint pixel_shader = create_and_compile_shader(GL_FRAGMENT_SHADER, fragment_shader_filename);
	assert(vertex_shader != 0 && pixel_shader != 0);
	
	GLint prog = glCreateProgram();
	glAttachShader(prog, vertex_shader);
	glAttachShader(prog, pixel_shader);
	glLinkProgram(prog);
	
	GLint result = GL_TRUE;
	glGetProgramiv(prog, GL_LINK_STATUS, &result);
	if (result == GL_FALSE){
		char buffer[1024];
		glGetProgramInfoLog(prog, 1024, NULL, buffer);
		printf("vertex and pixel shader linking faild:\n%s\n", buffer);
		glDeleteShader(vertex_shader);
		glDeleteShader(pixel_shader);
		return 0;
	}
	
	printf("compiled program %s %s\n", vertex_shader_filename, fragment_shader_filename);
	
	// Enum attribs
	GLint active_attrib_count = 0;
	glGetProgramiv(prog, GL_ACTIVE_ATTRIBUTES, &active_attrib_count);
	printf("%d attribs:\n", active_attrib_count);
	for(size_t i = 0; i < active_attrib_count; i++){
		char buffer[512];
		GLint size;
		GLenum type;
		glGetActiveAttrib(prog, i, 512, NULL, &size, &type, buffer);
		printf("- \"%s\": size %d, type %d\n", buffer, size, type);
	}
	
	// Enum uniforms
	GLint active_uniform_count = 0;
	glGetProgramiv(prog, GL_ACTIVE_UNIFORMS, &active_uniform_count);
	printf("%d uniforms:\n", active_uniform_count);
	for(size_t i = 0; i < active_uniform_count; i++){
		char buffer[512];
		GLint size;
		GLenum type;
		glGetActiveUniform(prog, i, 512, NULL, &size, &type, buffer);
		printf("- \"%s\": size %d, type %d\n", buffer, size, type);
	}
	
	return prog;
}

/**
 * Destorys the specified program and all shaders attached to it.
 */
static void delete_program_and_shaders(GLuint program){
	GLint shader_count = 0;
	glGetProgramiv(program, GL_ATTACHED_SHADERS, &shader_count);
	
	GLuint shaders[shader_count];
	glGetAttachedShaders(program, shader_count, NULL, shaders);
	
	glDeleteProgram(program);
	for(size_t i = 0; i < shader_count; i++)
		glDeleteShader(shaders[i]);
}

/**
 * Checks if an OpenGL extention is avaialbe.
 */
static bool gl_ext_present(const char *ext_name){
	GLint ext_count;
	glGetIntegerv(GL_NUM_EXTENSIONS, &ext_count);
	for(size_t i = 0; i < ext_count; i++){
		if ( strcmp((const char*)glGetStringi(GL_EXTENSIONS, i), ext_name) == 0 )
			return true;
	}
	return false;
}