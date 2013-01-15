#include <stdlib.h>
#include <stdio.h>

#include "viewport.h"

viewport_p vp_new(ivec2_t world_default_size, float scale_base, float scale_exp){
	viewport_p vp = malloc(sizeof(viewport_t));
	
	vp->pos = (ivec2_t){0, 0};
	vp->subpixel_pos = (vec2_t){0, 0};
	vp->world_default_size = world_default_size;
	
	vp->scale_base = scale_base;
	vp->scale_exp = scale_exp;
	
	vp->screen_space_objects = object_tree_new((object_t){});
	return vp;
}

void vp_destroy(viewport_p vp){
	object_tree_destroy(vp->screen_space_objects);
	free(vp);
}

void vp_changed(viewport_p vp){
	float aspect_ratio = (float)vp->screen_size.x / vp->screen_size.y;
	vp->scale = vp_scale_for(vp, vp->scale_exp);
	//printf("viewport: aspect ratio %f, scale base: %f, scale exp: %f, scale %f\n", aspect_ratio, vp->scale_base, vp->scale_exp, vp->scale);
	
	if (aspect_ratio > 1) {
		// Landscape format, use world_default_size.y as minimal height
		vp->world_size.y = vp->world_default_size.y * vp->scale;
		vp->world_size.x = vp->world_size.y * aspect_ratio;
	} else {
		// Portrait format, use world_default_size.x as minimal width
		vp->world_size.x = vp->world_default_size.x * vp->scale;
		vp->world_size.y = vp->world_size.x / aspect_ratio;
	}
	
	// Calculate matrix to convert screen coords back to world coords
	float sx = vp->world_size.x / (float)vp->screen_size.x;
	float sy = -vp->world_size.y / (float)vp->screen_size.y;
	float tx = -0.5 * vp->world_size.x + vp->pos.x + vp->subpixel_pos.x;
	float ty = 0.5 * vp->world_size.y + vp->pos.y + vp->subpixel_pos.y;
	m3_transpose(vp->screen_to_world, (mat3_t){
		sx, 0, tx,
		0, sy, ty,
		0, 0, 1
	});
	
	// World to normal matrix
	sx = 2.0 / vp->world_size.x;
	sy = -2.0 / vp->world_size.y;
	tx = -(vp->pos.x + vp->subpixel_pos.x) * (2.0 / vp->world_size.x);
	ty = -(vp->pos.y + vp->subpixel_pos.y) * (2.0 / vp->world_size.y);
	m3_transpose(vp->world_to_normal, (mat3_t){
		sx, 0, tx,
		0, sy, ty,
		0, 0, 1
	});
	
	// World to screen matrix
	sx = vp->screen_size.x / vp->world_size.x;
	sy = vp->screen_size.y / vp->world_size.y;
	tx = -(vp->pos.x + vp->subpixel_pos.x) * sx + 0.5 * vp->screen_size.x;
	ty = -(vp->pos.y + vp->subpixel_pos.y) * sy + 0.5 * vp->screen_size.y;
	m3_transpose(vp->world_to_screen, (mat3_t){
		sx, 0, tx,
		0, sy, ty,
		0, 0, 1
	});
}

void vp_screen_changed(viewport_p vp, ivec2_t new_screen_size){
	vp->screen_size = new_screen_size;
	
	// Adjust the world size to the new screen size. When the user resizes the viewport the size
	// of things stays the same but the user sees more. This is a more fitting behaviour for a window.
	vp->world_default_size = vp->screen_size;
	
	float sx = 2.0 / vp->screen_size.x;
	float sy = -2.0 / vp->screen_size.y;
	float tx = -1.0;
	float ty = 1.0;
	m3_transpose(vp->screen_to_normal, (mat3_t){
		sx, 0, tx,
		0, sy, ty,
		0, 0, 1
	});
	
	vp_changed(vp);
}

float vp_scale_for(viewport_p vp, float exp){
	return powf(vp->scale_base, exp);
}

irect_t vp_vis_world_rect(viewport_p vp){
	uint64_t w = vp->world_size.x * vp->scale;
	uint64_t h = vp->world_size.y * vp->scale;
	return (irect_t){
		.x = vp->pos.x - w / 2, .y = vp->pos.y - h / 2,
		.w = w, .h = h
	};
}