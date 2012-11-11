#pragma once

typedef struct { float r, g, b, a; } color_t;

GLint create_and_compile_shader(GLenum shader_type, const char *filename);
GLuint load_and_link_program(const char *vertex_shader_filename, const char *fragment_shader_filename);
void delete_program_and_shaders(GLuint program);

float rand_in(float lower, float upper);