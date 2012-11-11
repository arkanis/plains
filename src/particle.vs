#version 120

// vertex and cursor positions in screen space
attribute vec2 pos;

uniform mat3 transform;
uniform mat3 to_norm;

void main(){
	gl_Position.xyz = (to_norm * transform) * vec3(pos, 1);
	gl_Position.w = 1;
}