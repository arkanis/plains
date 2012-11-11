#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <assert.h>

#include <stdio.h>
#include <stdlib.h>
#define __USE_XOPEN 1
#include <math.h>

#include <SDL/SDL.h>

#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>

#include "common.h"
#include "math.h"
#include "viewport.h"



/*

Resource actions:
- load
- new
- destroy
- unload

Units:
- World space: meter [m]

Pipeline:
- transform * local coords -> world coords
- camera * world coords -> normalized device coords
- viewport * normalized device coords -> screen coords

*/

// Viewport of the renderer. Data from the viewport is used by other components.
viewport_p viewport;

//
// Grid
//
GLuint grid_prog, grid_vertex_buffer;
// Space between grid lines in world units
vec2_t grid_default_spacing = {1, 1};

void grid_load(){
	grid_prog = load_and_link_program("grid.vs", "grid.ps");
	assert(grid_prog != 0);
	
	glGenBuffers(1, &grid_vertex_buffer);
	assert(grid_vertex_buffer != 0);
	glBindBuffer(GL_ARRAY_BUFFER, grid_vertex_buffer);
	
	// Rectangle
	const float vertecies[] = {
		1, 1,
		-1, 1,
		-1, -1,
		1, -1
	};
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertecies), vertecies, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void grid_unload(){
	glDeleteBuffers(1, &grid_vertex_buffer);
	delete_program_and_shaders(grid_prog);
}

void grid_draw(){
	vec2_t grid_spacing = (vec2_t){
		grid_default_spacing.x * viewport->world_to_screen[0],
		grid_default_spacing.y * viewport->world_to_screen[4]
	};
	vec2_t grid_offset = (vec2_t){
		viewport->pos.x * viewport->world_to_screen[0],
		viewport->pos.y * viewport->world_to_screen[4]
	};
	
	glUseProgram(grid_prog);
	glBindBuffer(GL_ARRAY_BUFFER, grid_vertex_buffer);
	
	GLint pos_attrib = glGetAttribLocation(grid_prog, "pos");
	glEnableVertexAttribArray(pos_attrib);
	glVertexAttribPointer(pos_attrib, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, 0);
	
	glUniform4f( glGetUniformLocation(grid_prog, "color"), 0, 0, 0.5, 1 );
	glUniform2f( glGetUniformLocation(grid_prog, "grid_spacing"), grid_spacing.x, grid_spacing.y );
	glUniform2f( glGetUniformLocation(grid_prog, "grid_offset"), grid_offset.x, grid_offset.y );
	glUniform1f( glGetUniformLocation(grid_prog, "vp_scale_exp"), viewport->scale_exp );
	glUniform2f( glGetUniformLocation(grid_prog, "screen_size"), viewport->screen_size.x, viewport->screen_size.y );
	glDrawArrays(GL_QUADS, 0, 4);
	
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glUseProgram(0);
}


//
// Cursor
//
vec2_t cursor_pos = {0, 0};
GLuint cursor_prog, cursor_vertex_buffer;

void cursor_load(){
	cursor_prog = load_and_link_program("cursor.vs", "cursor.ps");
	assert(cursor_prog != 0);
	
	glGenBuffers(1, &cursor_vertex_buffer);
	assert(cursor_vertex_buffer != 0);
	glBindBuffer(GL_ARRAY_BUFFER, cursor_vertex_buffer);
	
	// Rectangle
	const float vertecies[] = {
		5, 5,
		-5, 5,
		-5, -5,
		5, -5
	};
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertecies), vertecies, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void cursor_unload(){
	glDeleteBuffers(1, &cursor_vertex_buffer);
	delete_program_and_shaders(cursor_prog);
}

void cursor_draw(){
	//printf("cursor pos: x %f y %f\n", cursor_pos.x, cursor_pos.y);
	
	glUseProgram(cursor_prog);
	glBindBuffer(GL_ARRAY_BUFFER, cursor_vertex_buffer);
	
	GLint pos_attrib = glGetAttribLocation(cursor_prog, "pos");
	glEnableVertexAttribArray(pos_attrib);
	glVertexAttribPointer(pos_attrib, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, 0);
	
	GLint cursor_pos_uni = glGetUniformLocation(cursor_prog, "cursor_pos");
	assert(cursor_pos_uni != -1);
	glUniform2f(cursor_pos_uni, cursor_pos.x, cursor_pos.y);
	
	GLint color_uni = glGetUniformLocation(cursor_prog, "color");
	assert(color_uni != -1);
	glUniform4f(color_uni, 1, 1, 1, 1);
	
	GLint projection_uni = glGetUniformLocation(cursor_prog, "projection");
	assert(projection_uni != -1);
	glUniformMatrix3fv(projection_uni , 1, GL_FALSE, viewport->screen_to_normal);
	
	glDrawArrays(GL_QUADS, 0, 4);
	
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glUseProgram(0);
}


//
// Particles
//
typedef struct {
	vec2_t pos, vel, force;  // m, m_s, m_s2
	float mass;  // kg
	int flags;
} particle_t, *particle_p;

#define PARTICLE_TRAVERSED 1<<0

particle_p particles = NULL;
size_t particle_count;
GLuint particle_prog, particle_vertex_buffer;

typedef struct {
	particle_p p1, p2;
	float length;  // m
	int flags;
} beam_t, *beam_p;

#define BEAM_TRAVERSED 1<<0
#define BEAM_FOLLOWED 1<<1

beam_p beams = NULL;
size_t beam_count;
GLuint beam_prog, beam_vertex_buffer;

void particles_load(){
	beam_prog = load_and_link_program("unit.vs", "unit.ps");
	assert(beam_prog != 0);
	
	glGenBuffers(1, &beam_vertex_buffer);
	assert(beam_vertex_buffer != 0);
	
	particle_prog = load_and_link_program("particle.vs", "particle.ps");
	assert(particle_prog != 0);
	
	glGenBuffers(1, &particle_vertex_buffer);
	assert(particle_vertex_buffer != 0);
	glBindBuffer(GL_ARRAY_BUFFER, particle_vertex_buffer);
	
	const float vertecies[] = {
		// Rectangle
		0.5, 0.5,
		-0.5, 0.5,
		-0.5, -0.5,
		0.5, -0.5,
		// Arrow: -->
		1, 0,
		0.25, 0.25,
		0, -1,
		0.25, -0.25
	};
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertecies), vertecies, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	
	// Create test particles
	/*
	particle_count = 4;
	particles = realloc(particles, sizeof(particle_t) * particle_count);
	
	particles[0] = (particle_t){
		.pos = (vec2_t){ -1, -1 },
		.vel = (vec2_t){ 0, 0 },
		.force = (vec2_t){0, 0},
		.mass = 1
	};
	particles[1] = (particle_t){
		.pos = (vec2_t){ 0, 0 },
		.vel = (vec2_t){ 0, 0 },
		.force = (vec2_t){0, 0},
		.mass = 1
	};
	particles[2] = (particle_t){
		.pos = (vec2_t){ 2, 2 },
		.vel = (vec2_t){ 0, 0 },
		.force = (vec2_t){0, 0},
		.mass = 1
	};
	particles[3] = (particle_t){
		.pos = (vec2_t){ 2, 0 },
		.vel = (vec2_t){ 0, 0 },
		.force = (vec2_t){0, 0},
		.mass = 1
	};
	*/
	/*
	srand(5);
	for(size_t i = 0; i < particle_count; i++){
		particles[i] = (particle_t){
			.pos = (vec2_t){ rand_in(-10, 10), rand_in(-10, 10) },
			.vel = (vec2_t){ rand_in(-2, 2), rand_in(-2, 2) },
			.force = (vec2_t){0, 0},
			.mass = 1
		};
	}
	*/
	
	// Create test beams
	inline particle_t particle_at(float x, float y){
		return (particle_t){
			.pos = (vec2_t){ x, y },
			.vel = (vec2_t){ 0, 0 },
			.force = (vec2_t){0, 0},
			.mass = 1
		};
	}
	
	particle_count = 16;
	particles = realloc(particles, sizeof(particle_t) * particle_count);
	
	particles[0] = particle_at(0, 0);
	particles[1] = particle_at(0, 2);
	particles[2] = particle_at(0, 4);
	particles[3] = particle_at(0, 6);
	particles[4] = particle_at(-1, -1);
	particles[5] = particle_at(-1, 1);
	particles[6] = particle_at(-1, 3);
	particles[7] = particle_at(-1, 5);
	particles[8] = particle_at(-2, -1);
	particles[9] = particle_at(-2, 1);
	particles[10] = particle_at(-2, 3);
	particles[11] = particle_at(-2, 5);
	particles[12] = particle_at(-3, 0);
	particles[13] = particle_at(-3, 2);
	particles[14] = particle_at(-3, 4);
	particles[15] = particle_at(-3, 6);
	
	inline beam_t beam_from_to(size_t p1_index, size_t p2_index){
		return (beam_t){
			.p1 = &particles[p1_index],
			.p2 = &particles[p2_index],
			.length = v2_length( v2_sub(particles[p2_index].pos, particles[p1_index].pos) )
		};
	}
	
	beam_count = 24;
	beams = realloc(beams, sizeof(beam_t) * beam_count);
	
	beams[0] = beam_from_to(0, 1);
	beams[1] = beam_from_to(1, 2);
	beams[2] = beam_from_to(2, 3);
	beams[3] = beam_from_to(0, 4);
	beams[4] = beam_from_to(0, 5);
	beams[5] = beam_from_to(1, 5);
	beams[6] = beam_from_to(1, 6);
	beams[7] = beam_from_to(2, 6);
	beams[8] = beam_from_to(2, 7);
	beams[9] = beam_from_to(3, 7);
	
	beams[10] = beam_from_to(4, 8);
	beams[11] = beam_from_to(5, 9);
	beams[12] = beam_from_to(6, 10);
	beams[13] = beam_from_to(7, 11);
	
	beams[14] = beam_from_to(12, 13);
	beams[15] = beam_from_to(13, 14);
	beams[16] = beam_from_to(14, 15);
	beams[17] = beam_from_to(12, 8);
	beams[18] = beam_from_to(12, 9);
	beams[19] = beam_from_to(13, 9);
	beams[20] = beam_from_to(13, 10);
	beams[21] = beam_from_to(14, 10);
	beams[22] = beam_from_to(14, 11);
	beams[23] = beam_from_to(15, 11);
}

void particles_unload(){
	free(particles);
	particles = NULL;
	
	glDeleteBuffers(1, &particle_vertex_buffer);
	delete_program_and_shaders(particle_prog);
}

void particles_draw(){
	// Draw particles
	glUseProgram(particle_prog);
	glBindBuffer(GL_ARRAY_BUFFER, particle_vertex_buffer);
	
	GLint pos_attrib = glGetAttribLocation(particle_prog, "pos");
	assert(pos_attrib != -1);
	glEnableVertexAttribArray(pos_attrib);
	glVertexAttribPointer(pos_attrib, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, 0);
	
	GLint color_uni = glGetUniformLocation(particle_prog, "color");
	GLint to_norm_uni = glGetUniformLocation(particle_prog, "to_norm");
	GLint trans_uni = glGetUniformLocation(particle_prog, "transform");
	assert(color_uni != -1);
	assert(to_norm_uni != -1);
	assert(trans_uni != -1);
	
	glUniform4f(color_uni, 0, 1, 0, 1 );
	glUniformMatrix3fv(to_norm_uni, 1, GL_FALSE, viewport->world_to_normal);
	
	for(size_t i = 0; i < particle_count; i++){
		glUniformMatrix3fv(trans_uni, 1, GL_TRUE, (float[9]){
			1, 0, particles[i].pos.x,
			0, 1, particles[i].pos.y,
			0, 0, 1
		});
		glDrawArrays(GL_QUADS, 0, 4);
	}
	
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glUseProgram(0);
	
	
	// Draw beams
	glUseProgram(beam_prog);
	glBindBuffer(GL_ARRAY_BUFFER, beam_vertex_buffer);
	
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 4 * beam_count, NULL, GL_STATIC_DRAW);
	float *vertex_buffer = glMapBuffer(GL_ARRAY_BUFFER, GL_READ_WRITE);
	for(size_t i = 0; i < beam_count; i++){
		vertex_buffer[i*4+0] = beams[i].p1->pos.x;
		vertex_buffer[i*4+1] = beams[i].p1->pos.y;
		vertex_buffer[i*4+2] = beams[i].p2->pos.x;
		vertex_buffer[i*4+3] = beams[i].p2->pos.y;
		//printf("beam %zu: from %f/%f to %f/%f\n", i, beams[i].p1->pos.x, beams[i].p1->pos.y, beams[i].p2->pos.x, beams[i].p2->pos.y);
	}
	/*
	for(size_t i = 0; i < 4 * beam_count; i++)
		printf("vb[%zu]: %f\n", i, vertex_buffer[i]);
	*/
	glUnmapBuffer(GL_ARRAY_BUFFER);
	
	pos_attrib = glGetAttribLocation(beam_prog, "pos");
	assert(pos_attrib != -1);
	glEnableVertexAttribArray(pos_attrib);
	glVertexAttribPointer(pos_attrib, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, 0);
	
	glUniform4f( glGetUniformLocation(beam_prog, "color"), 1, 1, 1, 1 );
	glUniformMatrix3fv( glGetUniformLocation(beam_prog, "to_norm"), 1, GL_FALSE, viewport->world_to_normal);
	
	glDrawArrays(GL_LINES, 0, beam_count * 2);
	
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glUseProgram(0);
}


//
// Renderer
//
void renderer_resize(uint16_t window_width, uint16_t window_height){
	SDL_SetVideoMode(window_width, window_height, 24, SDL_OPENGL | SDL_RESIZABLE);
	glViewport(0, 0, window_width, window_height);
	vp_screen_changed(viewport, window_width, window_height);
}

void renderer_load(uint16_t window_width, uint16_t window_height, const char *title){
	// Create window
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_WM_SetCaption(title, NULL);
	
	// Initialize viewport structure
	viewport = vp_new((vec2_t){10, 10}, 2);
	renderer_resize(window_width, window_height);
	
	// Enable alpha blending
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	
	glEnable(GL_LINE_SMOOTH);
	glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
}

void renderer_unload(){
	vp_destroy(viewport);
}

void renderer_draw(){
	glClearColor(0, 0, 0, 1.0);
	glClear(GL_COLOR_BUFFER_BIT);
	
	grid_draw();
	particles_draw();
	cursor_draw();
}


//
// Simulation
//
ssize_t sim_grabbed_particle_idx = -1;

typedef void (*particle_func_t)(particle_p particle, particle_p from);
typedef void (*beam_func_t)(beam_p beam, particle_p from, particle_p to);

/**
 * Do a broad iteration. That is iterate all connected beams before following the first beam to a new
 * particle. This is necessary for the forces to propagate from the start particle outwards. Otherwise
 * a depth first iteration might finish most particles "from behind" missing most of the forces caused
 * by the initial particle.
 */
void sim_traverse_particles(particle_p p, particle_func_t particle_func){
	assert(p != NULL && particle_func != NULL);
	
	particle_func(p, NULL);
	p->flags |= PARTICLE_TRAVERSED;
	
	size_t traversed_particles;
	do {
		traversed_particles = 0;
		
		for(size_t i = 0; i < beam_count; i++){
			beam_p beam = &beams[i];
			
			if ( (beam->p1->flags & PARTICLE_TRAVERSED) && !(beam->p2->flags & PARTICLE_TRAVERSED) ) {
				particle_func(beam->p2, beam->p1);
				beam->p2->flags |= PARTICLE_TRAVERSED;
				traversed_particles++;
			} else if ( (beam->p2->flags & PARTICLE_TRAVERSED) && !(beam->p1->flags & PARTICLE_TRAVERSED) ) {
				particle_func(beam->p1, beam->p2);
				beam->p1->flags |= PARTICLE_TRAVERSED;
				traversed_particles++;
			}
		}
	} while (traversed_particles > 0);
	
	/*
	for(size_t i = 0; i < beam_count; i++){
		beam_p beam = &beams[i];
		if ( (beam->flags & BEAM_TRAVERSED) == 0 ) {
			if (beam->p1 == p) {
				beam_func(beam, p, beam->p2);
				beam->flags |= BEAM_TRAVERSED;
			} else if (beam->p2 == p) {
				beam_func(beam, p, beam->p1);
				beam->flags |= BEAM_TRAVERSED;
			}
		}
	}
	
	for(size_t i = 0; i < beam_count; i++){
		beam_p beam = &beams[i];
		if ( ((beam->flags & BEAM_FOLLOWED) == 0) && (beam->p1 == p || beam->p2 == p) ){
			beam->flags |= BEAM_FOLLOWED;
			sim_traverse( (beam->p1 == p ? beam->p2 : beam->p1), beam_func, particle_func);
		}
	}
	
	particle_func(p);
	*/
}

void sim_traverse_beams(particle_p p, bool mark, beam_func_t beam_func){
	assert(p != NULL && beam_func != NULL);
	
	for(size_t i = 0; i < beam_count; i++){
		beam_p beam = &beams[i];
		if ( !(beam->flags & BEAM_TRAVERSED) ) {
			if (beam->p1 == p) {
				beam_func(beam, p, beam->p2);
				if (mark)
					beam->flags |= BEAM_TRAVERSED;
			} else if (beam->p2 == p) {
				beam_func(beam, p, beam->p1);
				if (mark)
					beam->flags |= BEAM_TRAVERSED;
			}
		}
	}	
}

void sim_clear_traverse_flags(){
	for(size_t i = 0; i < beam_count; i++)
		beams[i].flags = 0;
	for(size_t i = 0; i < particle_count; i++)
		particles[i].flags = 0;
}

void sim_propagate_forces(){
	sim_clear_traverse_flags();
	
	void particle_func(particle_p particle, particle_p particle_from){
		printf("iterating particle %p\n", particle);
		
		float scalar_sum = 0;
		void scalar_summer(beam_p beam, particle_p from, particle_p to){
			scalar_sum += v2_sprod( v2_norm(v2_sub(to->pos, from->pos)), v2_norm(particle->force) );
		}
		sim_traverse_beams(particle, false, scalar_summer);
		printf("scalar sum: %f\n", scalar_sum);
		
		if (scalar_sum > 0){
			void beam_func(beam_p beam, particle_p from, particle_p to){
				printf("iterating beam from %p to %p\n", from, to);
				float proj = v2_sprod( v2_norm(v2_sub(to->pos, from->pos)), v2_norm(particle->force) );
				to->force = v2_muls(particle->force, proj / scalar_sum);
			}
			sim_traverse_beams(particle, true, beam_func);
		}
		
		if (particle_from != NULL)
			particle->force = particle_from->force;
	}
	
	sim_traverse_particles(&particles[sim_grabbed_particle_idx], particle_func);
}

void simulate(float dt){
	for(size_t i = 0; i < particle_count; i++){
		/*
		a = f / m;
		v = v + a * dt;
		s = s + v * dt;
		*/
		particle_p p = &particles[i];
		
		vec2_t acl;
		acl.x = p->force.x / p->mass;
		acl.y = p->force.y / p->mass;
		p->vel.x += acl.x * dt;
		p->vel.y += acl.y * dt;
		p->pos.x += p->vel.x * dt;
		p->pos.y += p->vel.y * dt;
	}
}

void sim_apply_force(){
	vec2_t world_cursor = m3_v2_mul(viewport->screen_to_world, cursor_pos);
	
	// Find nearest particle
	size_t closest_idx = 0;
	float closest_dist = INFINITY;
	vec2_t to_closest;
	for(size_t i = 0; i < particle_count; i++){
		vec2_t to_particle = v2_sub(particles[i].pos, world_cursor);
		float dist = v2_length(to_particle);
		if (dist < closest_dist){
			closest_idx = i;
			closest_dist = dist;
			to_closest = to_particle;
		}
	}
	
	particles[closest_idx].force = to_closest;
	sim_grabbed_particle_idx = closest_idx;
	sim_propagate_forces();
}

void sim_retain_force(){
	particles[sim_grabbed_particle_idx].force = (vec2_t){0, 0};
	sim_propagate_forces();
	sim_grabbed_particle_idx = -1;
}


int main(int argc, char **argv){
	uint32_t cycle_duration = 1.0 / 60.0 * 1000;
	uint16_t win_w = 640, win_h = 480;
	
	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER);
	renderer_load(win_w, win_h, "Grid");
	
	grid_load();
	cursor_load();
	particles_load();
	
	SDL_Event e;
	bool quit = false, viewport_grabbed = false;
	uint32_t ticks = SDL_GetTicks();
	
	while (!quit) {
		while ( SDL_PollEvent(&e) ) {
			switch(e.type){
				case SDL_QUIT:
					quit = true;
					break;
				case SDL_VIDEORESIZE:
					renderer_resize(e.resize.w, e.resize.h);
					break;
				case SDL_KEYUP:
					switch(e.key.keysym.sym){
						case SDLK_LEFT:
							viewport->pos.x -= 1;
							vp_changed(viewport);
							break;
						case SDLK_RIGHT:
							viewport->pos.x += 1;
							vp_changed(viewport);
							break;
						case SDLK_UP:
							viewport->pos.y += 1;
							vp_changed(viewport);
							break;
						case SDLK_DOWN:
							viewport->pos.y -= 1;
							vp_changed(viewport);
							break;
					}
					break;
				case SDL_MOUSEMOTION:
					cursor_pos.x = e.motion.x;
					cursor_pos.y = e.motion.y;
					
					//vec2_t world_cursor = m3_v2_mul(viewport->screen_to_world, cursor_pos);
					//printf("world cursor: %f %f\n", world_cursor.x, world_cursor.y);
					//particles[0].pos = world_cursor;
					
					if (viewport_grabbed){
						// Only use the scaling factors from the current screen to world matrix. Since we work
						// with deltas here the offsets are not necessary (in fact would destroy the result).
						viewport->pos.x += -viewport->screen_to_world[0] * e.motion.xrel;
						viewport->pos.y += -viewport->screen_to_world[4] * e.motion.yrel;
						vp_changed(viewport);
					}
					
					break;
				case SDL_MOUSEBUTTONDOWN:
					switch(e.button.button){
						case SDL_BUTTON_LEFT:
							sim_apply_force();
							break;
						case SDL_BUTTON_MIDDLE:
							viewport_grabbed = true;
							break;
						case SDL_BUTTON_RIGHT:
							break;
						case SDL_BUTTON_WHEELUP:
							break;
						case SDL_BUTTON_WHEELDOWN:
							break;
					}
					break;
				case SDL_MOUSEBUTTONUP:
					switch(e.button.button){
						case SDL_BUTTON_LEFT:
							sim_retain_force();
							break;
						case SDL_BUTTON_MIDDLE:
							viewport_grabbed = false;
							break;
						case SDL_BUTTON_RIGHT:
							break;
						case SDL_BUTTON_WHEELUP:
							viewport->scale_exp -= 0.1;
							{
								vec2_t world_cursor = m3_v2_mul(viewport->screen_to_world, cursor_pos);
								float new_scale = vp_scale_for(viewport, viewport->scale_exp);
								viewport->pos = (vec2_t){
									world_cursor.x + (viewport->pos.x - world_cursor.x) * (new_scale / viewport->scale),
									world_cursor.y + (viewport->pos.y - world_cursor.y) * (new_scale / viewport->scale)
								};
							}
							vp_changed(viewport);
							break;
						case SDL_BUTTON_WHEELDOWN:
							viewport->scale_exp += 0.1;
							{
								vec2_t world_cursor = m3_v2_mul(viewport->screen_to_world, cursor_pos);
								float new_scale = vp_scale_for(viewport, viewport->scale_exp);
								viewport->pos = (vec2_t){
									world_cursor.x + (viewport->pos.x - world_cursor.x) * (new_scale / viewport->scale),
									world_cursor.y + (viewport->pos.y - world_cursor.y) * (new_scale / viewport->scale)
								};
							}
							vp_changed(viewport);
							break;
					}
					break;
			}
		}
		
		renderer_draw();
		SDL_GL_SwapBuffers();
		simulate(cycle_duration / 1000.0);
		
		int32_t duration = cycle_duration - (SDL_GetTicks() - ticks);
		if (duration > 0)
			SDL_Delay(duration);
		ticks = SDL_GetTicks();
	}
	
	// Cleanup time
	particles_unload();
	cursor_unload();
	grid_unload();
	renderer_unload();
	
	SDL_Quit();
	return 0;
}