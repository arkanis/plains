#pragma once
#include <stdint.h>


typedef struct {
	int64_t x, y;
	int8_t z;
	uint64_t width, height;
	
	size_t client_idx;
	void* client_private;
} object_t, *object_p;