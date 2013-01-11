#pragma once

#include "layer.h"

typedef struct {
	size_t layer_idx;
	float scale_exp;
	int64_t x, y;
	uint64_t w, h;
	int shm_fd;
	uint64_t req_seq;
	uint8_t flags;
} draw_request_t, *draw_request_p;

#define DRAW_REQUEST_ANSWERED     1 << 1
#define DRAW_REQUEST_PLACEHOLDER  1 << 2
#define DRAW_REQUEST_DONE         1 << 3