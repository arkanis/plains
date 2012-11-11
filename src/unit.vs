#version 120

uniform mat3 to_norm;
attribute vec2 pos;

void main(){
	gl_Position.xyz = to_norm * vec3(pos, 1);
	gl_Position.w = 1;
}