#include <stddef.h>
#include <math.h>

#include "math.h"


void m3_transpose(mat3_t mr, mat3_t ma){
	mr[0] = ma[0]; mr[3] = ma[1]; mr[6] = ma[2];
	mr[1] = ma[3]; mr[4] = ma[4]; mr[7] = ma[5];
	mr[2] = ma[6]; mr[5] = ma[7]; mr[8] = ma[8];
}

void m3_identity(mat3_t mat){
	mat[0] = 1; mat[3] = 0; mat[6] = 0;
	mat[1] = 0; mat[4] = 1; mat[7] = 0;
	mat[2] = 0; mat[5] = 0; mat[8] = 1;
}

float m3_det(mat3_t mat){
	float det;
	
	det	= mat[0] * ( mat[4]*mat[8] - mat[7]*mat[5] )
		- mat[1] * ( mat[3]*mat[8] - mat[6]*mat[5] )
		+ mat[2] * ( mat[3]*mat[7] - mat[6]*mat[4] );
	
	return det;
}


void m3_inverse(mat3_t mr, mat3_t ma){
	float det = m3_det(ma);
	
	if ( fabs( det ) < 0.0005 )
	{
		m3_identity( ma );
		return;
	}
	
	mr[0] =    ma[4]*ma[8] - ma[5]*ma[7]   / det;
	mr[1] = -( ma[1]*ma[8] - ma[7]*ma[2] ) / det;
	mr[2] =    ma[1]*ma[5] - ma[4]*ma[2]   / det;
	
	mr[3] = -( ma[3]*ma[8] - ma[5]*ma[6] ) / det;
	mr[4] =    ma[0]*ma[8] - ma[6]*ma[2]   / det;
	mr[5] = -( ma[0]*ma[5] - ma[3]*ma[2] ) / det;
	
	mr[6] =    ma[3]*ma[7] - ma[6]*ma[4]   / det;
	mr[7] = -( ma[0]*ma[7] - ma[6]*ma[1] ) / det;
	mr[8] =    ma[0]*ma[4] - ma[1]*ma[3]   / det;
}

void m3_m3_mul(mat3_t mr, mat3_t m1, mat3_t m2){
	for(size_t i = 0; i < 3; i++){
		for(size_t j = 0; j < 3; j++){
			mr[i*3 + j] = m1[i*3 + 0] * m2[0*3 + j] + m1[i*3 + 1] * m2[1*3 + j] + m1[i*3 + 2] * m2[2*3 + j];
		}
	}
}

vec3_t m3_v3_mul(mat3_t mat, vec3_t v){
	return (vec3_t){
		mat[0] * v.x + mat[3] * v.y + mat[6] * v.z,
		mat[1] * v.x + mat[4] * v.y + mat[7] * v.z,
		mat[2] * v.x + mat[5] * v.y + mat[8] * v.z
	};
}

vec2_t m3_v2_mul(mat3_t mat, vec2_t v){
	return (vec2_t){
		mat[0] * v.x + mat[3] * v.y + mat[6],
		mat[1] * v.x + mat[4] * v.y + mat[7]
	};
}