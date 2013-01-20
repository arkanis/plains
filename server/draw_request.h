#pragma once

#include "object.h"

typedef struct { float r, g, b, a; } color_t;
typedef struct {
	object_p object;
	
	// Stuff the renderer needs:
	
	// Coords and dimensions of the object to draw (actual space depends on the transform)
	int64_t wx, wy;
	uint64_t ww, wh;
	float* transform;
	
	// Size of the buffer and shared memory file descriptor
	uint64_t bw, bh;
	int shm_fd;
	color_t color;
	
	// Stuff the client needs to know what to draw:
	
	// scale and rect of the object that is visible and should be drawn (in object space)
	float scale;
	int64_t rx, ry;
	uint64_t rw, rh;
	
	uint64_t req_seq;
	uint8_t flags;
} draw_request_t, *draw_request_p;

#define DRAW_REQUEST_BUFFERED     1 << 1
#define DRAW_REQUEST_DONE         1 << 2