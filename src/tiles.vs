#version 120

attribute vec4 pos_and_tex;
uniform mat3 world_to_norm;

varying vec2 tex_coords;

void main(){
	gl_Position.xyz = world_to_norm * vec3(pos_and_tex.xy, 1);
	gl_Position.w = 1;
	
	tex_coords = pos_and_tex.zw;
}