#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>


/**
 * Loads and compiles a source code file as a shader.
 * 
 * Returns the shaders GL object id on success or 0 on error.
 */
GLint create_and_compile_shader(GLenum shader_type, const char *filename){
	int fd = open(filename, O_RDONLY, 0);
	struct stat file_stat;
	fstat(fd, &file_stat);
	char *code = mmap(NULL, file_stat.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
	
	GLint shader = glCreateShader(shader_type);
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


GLuint load_and_link_program(const char *vertex_shader_filename, const char *fragment_shader_filename){
	// Load shaders
	GLint vertex_shader = create_and_compile_shader(GL_VERTEX_SHADER, vertex_shader_filename);
	GLint pixel_shader = create_and_compile_shader(GL_FRAGMENT_SHADER, fragment_shader_filename);
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
void delete_program_and_shaders(GLuint program){
	GLint shader_count = 0;
	glGetProgramiv(program, GL_ATTACHED_SHADERS, &shader_count);
	
	GLuint shaders[shader_count];
	glGetAttachedShaders(program, shader_count, NULL, shaders);
	
	glDeleteProgram(program);
	for(size_t i = 0; i < shader_count; i++)
		glDeleteShader(shaders[i]);
}


//
// Utility functions
//

float rand_in(float lower, float upper){
	return (rand() / (float)RAND_MAX) * (upper - lower) + lower;
}