#pragma once

#include "object.h"

typedef struct { float r, g, b, a; } color_t;
typedef struct {
	int64_t x, y;
	object_p object;
	color_t color;
} draw_request_t, *draw_request_p;