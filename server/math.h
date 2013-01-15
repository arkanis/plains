#pragma once
#include <math.h>
#include <stdint.h>

typedef float mat4_t[16];
typedef float mat3_t[9];
typedef struct { float x, y; } vec2_t;
typedef struct { float x, y, z; } vec3_t;
typedef struct { int64_t x, y; } ivec2_t;
typedef struct { int64_t x, y; uint64_t w, h; } irect_t;


//
// Matrix arithmetic
//

void m3_transpose(mat3_t mr, mat3_t ma);
void m3_identity(mat3_t mat);
float m3_det(mat3_t mat);
void m3_inverse(mat3_t mr, mat3_t ma);
void m3_m3_mul(mat3_t mr, mat3_t m1, mat3_t m2);
vec3_t m3_v3_mul(mat3_t mat, vec3_t v);
vec2_t m3_v2_mul(mat3_t mat, vec2_t v);


//
// Float vector arithmetic
//

static inline vec2_t v2_add(vec2_t a, vec2_t b){
	return (vec2_t){
		a.x + b.x,
		a.y + b.y
	};
}

static inline vec2_t v2_adds(vec2_t a, float s){
	return (vec2_t){
		a.x + s,
		a.y + s
	};
}

static inline vec2_t v2_sub(vec2_t a, vec2_t b){
	return (vec2_t){
		a.x - b.x,
		a.y - b.y
	};
}

static inline vec2_t v2_subs(vec2_t a, float s){
	return (vec2_t){
		a.x - s,
		a.y - s
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

static inline vec2_t v2_divs(vec2_t a, float s){
	return (vec2_t){
		a.x / s,
		a.y / s
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


//
// Integer vector arithmetic
//

static inline ivec2_t iv2_add(ivec2_t a, ivec2_t b){
	return (ivec2_t){
		a.x + b.x,
		a.y + b.y
	};
}

static inline ivec2_t iv2_adds(ivec2_t a, int64_t s){
	return (ivec2_t){
		a.x + s,
		a.y + s
	};
}

static inline ivec2_t iv2_sub(ivec2_t a, ivec2_t b){
	return (ivec2_t){
		a.x - b.x,
		a.y - b.y
	};
}

static inline ivec2_t iv2_subs(ivec2_t a, int64_t s){
	return (ivec2_t){
		a.x - s,
		a.y - s
	};
}

static inline ivec2_t iv2_mul(ivec2_t a, ivec2_t b){
	return (ivec2_t){
		a.x * b.x,
		a.y * b.y
	};
}

static inline ivec2_t iv2_muls(ivec2_t a, int64_t s){
	return (ivec2_t){
		a.x * s,
		a.y * s
	};
}

static inline ivec2_t iv2_div(ivec2_t a, ivec2_t b){
	return (ivec2_t){
		a.x / b.x,
		a.y / b.y
	};
}

static inline ivec2_t iv2_divs(ivec2_t a, int64_t s){
	return (ivec2_t){
		a.x / s,
		a.y / s
	};
}


//
// Misc integer arithmetic
//

static inline uint64_t iceildiv(uint64_t a, uint64_t b){
	uint64_t result = a / b;
	if (a % b != 0)
		result++;
	return result;
}

static inline int64_t imin(int64_t a, int64_t b){
	return a < b ? a : b;
}

static inline int64_t imax(int64_t a, int64_t b){
	return a > b ? a : b;
}

static inline irect_t irect_intersection(irect_t a, irect_t b){
	// Use signed ints for all values so C does not get funny (unsigned) ideas in any calculations
	ivec2_t a1 = {a.x, a.y}, a2 = {a.x+a.w, a.y+a.h};
	ivec2_t b1 = {b.x, b.y}, b2 = {b.x+b.w, b.y+b.h};
	
	// Calculate points of intersection rect
	ivec2_t i1 = { imax(a1.x, b1.x), imax(a1.y, b1.y) };
	ivec2_t i2 = { imin(a2.x, b2.x), imin(a2.y, b2.y) };
	
	int64_t w = i2.x - i1.x, h = i2.y - i1.y;
	if (w < 0) w = 0;
	if (h < 0) h = 0;
	
	return (irect_t){ i1.x, i1.y, w, h };
}