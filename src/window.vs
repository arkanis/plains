#version 120

// window edge
attribute vec2 pos;

// window pos and size in world space
uniform vec4 pos_and_size;
uniform mat3 world_to_norm;

varying vec2 tex_coords;

void main(){
	vec2 screen_pos = pos_and_size.xy + pos * pos_and_size.zw;
	gl_Position.xyz = world_to_norm * vec3(screen_pos, 1);
	gl_Position.w = 1;
	
	// Flip the y axis right now because the image coord origin is in the top left edge
	tex_coords.x = pos.x * pos_and_size.z;
	tex_coords.y = pos_and_size.w - pos.y * pos_and_size.w;
}