#pragma once

#include "object.h"

typedef struct { float r, g, b, a; } color_t;
typedef struct {
	object_p object;
	color_t color;
} draw_request_t, *draw_request_p;