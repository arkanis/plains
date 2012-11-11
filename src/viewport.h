#pragma once
#include <stdint.h>
#include "math.h"

typedef struct {
	// Center of viewport in world space
	vec2_t pos;
	// Viewport scaling. scale = scale_base ^ scale_exp. This makes it easier to just increase or decrease the
	// scale exponent. This in turn gives more intuitive zooming behaviour.
	float scale_base, scale_exp, scale;
	
	// Size of the viewport on world space. The default size are the dimensions of the viewport with no scaling
	// applied (scale == 1). The x component is the min width, the y component the min height of viewport.  Min
	// width is used in landscape mode, min height in portrait mode.
	vec2_t world_size, world_default_size;
	// Transformations from world to normal and screen space
	mat3_t world_to_normal, world_to_screen;
	
	// Screen size in pixels
	vec2_t screen_size;
	// Transformations from screen to normal and world space
	mat3_t screen_to_normal, screen_to_world;
} viewport_t, *viewport_p;


viewport_p vp_new(vec2_t world_default_size, float scale_base);
void vp_destroy(viewport_p vp);
void vp_changed(viewport_p vp);
void vp_screen_changed(viewport_p vp, uint16_t screen_width, uint16_t screen_height);
float vp_scale_for(viewport_p vp, float exp);