#version 120

// vertex and cursor positions in screen space
attribute vec2 pos;
uniform vec2 cursor_pos;
uniform mat3 projection;

varying vec2 screen_coords;

void main(){
	screen_coords = cursor_pos + pos;
	gl_Position.xyz = projection * vec3(screen_coords, 1);
	gl_Position.w = 1;
}