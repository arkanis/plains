#pragma once
#include <stdint.h>
#include "math.h"
#include "object_tree.h"

typedef struct {
	// Center of viewport in world space
	ivec2_t pos;
	// In case we're zoomed beyond pixel accuracy this contains the subpixel position. Therefore the values
	// are between 0 and 1.
	vec2_t subpixel_pos;
	
	// Viewport scaling. scale = scale_base ^ scale_exp. This makes it easier to just increase or decrease the
	// scale exponent. This in turn gives more intuitive zooming behaviour.
	// We use float here because we need to ajust the scale (best through the scale_exp) to show a arbitrary
	// rect as fullscreen.
	float scale_base, scale_exp, scale;
	
	// Size of the viewport on world space. The default size are the dimensions of the viewport with no scaling
	// applied (scale == 1). The x component is the min width, the y component the min height of viewport.  Min
	// width is used in landscape mode, min height in portrait mode.
	ivec2_t world_size, world_default_size;
	// Transformations from world to normal and screen space
	mat3_t world_to_normal, world_to_screen;
	
	// Current screen size in pixels
	ivec2_t screen_size;
	// Transformations from screen to normal and world space
	mat3_t screen_to_normal, screen_to_world;
	
	object_tree_p screen_space_objects;
} viewport_t, *viewport_p;


viewport_p vp_new(uint16_t world_default_width, uint16_t world_default_height, float scale_base, float scale_exp);
void vp_destroy(viewport_p vp);
void vp_changed(viewport_p vp);
void vp_screen_changed(viewport_p vp, uint16_t width, uint16_t height);
float vp_scale_for(viewport_p vp, float exp);
irect_t vp_vis_world_rect(viewport_p vp);