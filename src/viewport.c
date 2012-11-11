#include <stdlib.h>

#include "viewport.h"

viewport_p vp_new(vec2_t world_default_size, float scale_base){
	viewport_p vp = malloc(sizeof(viewport_t));
	vp->pos = (vec2_t){0, 0};
	vp->world_default_size = world_default_size;
	vp->scale_base = scale_base;
	return vp;
}

void vp_destroy(viewport_p vp){
	free(vp);
}

void vp_changed(viewport_p vp){
	float aspect_ratio = vp->screen_size.x / vp->screen_size.y;
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
	float sx = vp->world_size.x / vp->screen_size.x;
	float sy = -vp->world_size.y / vp->screen_size.y;
	float tx = -0.5 * vp->world_size.x + vp->pos.x;
	float ty = 0.5 * vp->world_size.y + vp->pos.y;
	m3_transpose(vp->screen_to_world, (mat3_t){
		sx, 0, tx,
		0, sy, ty,
		0, 0, 1
	});
	
	sx = 2 / vp->world_size.x;
	sy = 2 / vp->world_size.y;
	tx = -vp->pos.x * (2 / vp->world_size.x);
	ty = -vp->pos.y * (2 / vp->world_size.y);
	m3_transpose(vp->world_to_normal, (mat3_t){
		sx, 0, tx,
		0, sy, ty,
		0, 0, 1
	});
	
	sx = vp->screen_size.x / vp->world_size.x;
	sy = vp->screen_size.y / vp->world_size.y;
	tx = -vp->pos.x * sx + 0.5 * vp->screen_size.x;
	ty = -vp->pos.y * sy + 0.5 * vp->screen_size.y;
	m3_transpose(vp->world_to_screen, (mat3_t){
		sx, 0, tx,
		0, sy, ty,
		0, 0, 1
	});
}

void vp_screen_changed(viewport_p vp, uint16_t screen_width, uint16_t screen_height){
	vp->screen_size = (vec2_t){ screen_width, screen_height };
	
	float sx = 2.0 / screen_width;
	float sy = -2.0 / screen_height;
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