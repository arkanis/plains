#pragma once
#include <stdint.h>


typedef struct {
	int64_t x, y;
	int8_t z;
	uint64_t width, height;
} object_t, *object_p;