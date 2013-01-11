#pragma once



typedef uint32_t page_idx_t;
#define PAGE_IDX_NONE UINT32_MAX

typedef struct {
	int16_t scale_idx;
	size_t page_count;
	page_idx_t pages[];
} page_segment_t, *page_segment_p;




typedef union {
	// When a page is allocated it points to the layer that uses the page
	layer_p layer;
	// If the page is not allocated it contains the number of the next unallocated
	// page (free list). A value of PAGE_IDX_NONE means that there is no further
	// unallocated page available.
	page_idx_t next_free_page;
} page_t, *page_p;

typedef struct {
	// Size of one page (width and height) in pixels
	uint16_t page_size;
	// Texture dimensions in pixels
	uint32_t width, height;
	
	// Number of the next free page or PAGE_IDX_NONE if there is no free page left
	page_idx_t next_free_page;
	// Head and tail of double linked list that manages the layers in recently used order
	layer_p first, last;
	
	size_t page_count;
	page_p pages;
} page_table_t, *page_table_p;




typedef struct {
	int16_t scale_exp;
	size_t page_count;
	page_idx_t pages[];
} layer_scale_t, *layer_scale_p;


typedef struct layer_s layer_t, *layer_p;

struct layer_s {
	int64_t x, y;
	uint64_t width, height;
	int32_t z;
	GLuint texture;
	/*
	// Double linked list used to easily get the least recently used layer
	layer_p prev, next;
	*/
	size_t scale_count;
	layer_scale_p scales;
};




page_table_p pt_new(uint32_t width, uint32_t height){
	page_table_p pt = malloc(sizeof(page_table_t));
	
}

void pt_destroy(page_table_p pt){
	free(pt);
}

/*

Layer:
- Collect texture coordinates

Page table:
- Allocate new pages
- Reuse old pages

*/