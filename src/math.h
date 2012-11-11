#pragma once
#include <math.h>

typedef float mat4_t[16];
typedef float mat3_t[9];
typedef struct { float x, y; } vec2_t;
typedef struct { float x, y, z; } vec3_t;

void m3_transpose(mat3_t mr, mat3_t ma);
void m3_identity(mat3_t mat);
float m3_det(mat3_t mat);
void m3_inverse(mat3_t mr, mat3_t ma);
void m3_m3_mul(mat3_t mr, mat3_t m1, mat3_t m2);
vec3_t m3_v3_mul(mat3_t mat, vec3_t v);
vec2_t m3_v2_mul(mat3_t mat, vec2_t v);

static inline vec2_t v2_add(vec2_t a, vec2_t b){
	return (vec2_t){
		a.x + b.x,
		a.y + b.y
	};
}

static inline vec2_t v2_sub(vec2_t a, vec2_t b){
	return (vec2_t){
		a.x - b.x,
		a.y - b.y
	};
}

static inline vec2_t v2_mul(vec2_t a, vec2_t b){
	return (vec2_t){
		a.x * b.x,
		a.y * b.y
	};
}

static inline vec2_t v2_muls(vec2_t a, float s){
	return (vec2_t){
		a.x * s,
		a.y * s
	};
}

static inline vec2_t v2_div(vec2_t a, vec2_t b){
	return (vec2_t){
		a.x / b.x,
		a.y / b.y
	};
}

static inline float v2_length(vec2_t v){
	return sqrtf(v.x*v.x + v.y*v.y);
}

static inline float v2_sprod(vec2_t a, vec2_t b){
	return a.x * b.x + a.y * b.y;
}

static inline vec2_t v2_norm(vec2_t v){
	float len = v2_length(v);
	if (len > 0){
		return (vec2_t){
			v.x / len,
			v.y / len
		};
	}
	
	return (vec2_t){ 0, 0 };
}