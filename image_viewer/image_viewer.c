#include <stdint.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdio.h>
// For a proper basename (GNU version)
#define _GNU_SOURCE
#include <string.h>
#include <libgen.h>

#include <math.h>
// For type constants of readdir()
#include <dirent.h>

#include <plains/client.h>
#include <fitz.h>
#include "stb_image.c"


//
// Metadata functions
//

typedef struct data_entry_s data_entry_t, *data_entry_p;

struct data_entry_s {
	const char* name;
	int64_t x, y;
	data_entry_p next;
};

typedef struct {
	const char* path;
	data_entry_p head;
} dir_metadata_t, *dir_metadata_p;

data_entry_p metadata_prepend(dir_metadata_p metadata){
	data_entry_p item = malloc(sizeof(data_entry_t));
	item->next = metadata->head;
	metadata->head = item;
	return item;
}

dir_metadata_p metadata_read(const char *path){
	dir_metadata_p metadata = malloc(sizeof(dir_metadata_t));
	metadata->path = path;
	metadata->head = NULL;
	
	FILE* f = fopen(path, "rb");
	if (f == NULL)
		return metadata;
	
	int64_t x, y;
	char name[1024];
	
	while( !feof(f) ){
		fscanf(f, "%1024[^\t] pos( %ld , %ld ) ", name, &x, &y);
		data_entry_p item = metadata_prepend(metadata);
		item->name = strdup(name);
		item->x = x;
		item->y = y;
	}
	
	fclose(f);
	return metadata;
}

void metadata_write(dir_metadata_p metadata){
	if (metadata == NULL)
		return;
	
	FILE* f = fopen(metadata->path, "wb");
	for(data_entry_p n = metadata->head; n; n = n->next){
		fprintf(f, "%s\tpos(%ld, %ld)\n", n->name, n->x, n->y);
	}
	fclose(f);
}

data_entry_p metadata_get(dir_metadata_p metadata, const char *path){
	if (metadata == NULL)
		return NULL;
	
	for(data_entry_p n = metadata->head; n; n = n->next){
		if ( strcmp(n->name, path) == 0 )
			return n;
	}
	
	return NULL;
}

void metadata_set(dir_metadata_p metadata, const char *path, int64_t x, int64_t y){
	data_entry_p d = metadata_get(metadata, path);
	if (d) {
		d->x = x;
		d->y = y;
	} else if (metadata) {
		data_entry_p item = metadata_prepend(metadata);
		item->name = path;
		item->x = x;
		item->y = y;
	}
}


//
// Misc functions
//

char* str_append(const char *a, const char *b){
	size_t alen = strlen(a), blen = strlen(b);
	char* dest = malloc(alen + blen + 1);
	strcpy(dest, a);
	strcpy(dest + alen, b);
	return dest;
}

char* str_tricat(const char *a, const char *b, const char *c){
	size_t len_a = strlen(a), len_b = strlen(b), len_c = strlen(c);
	char* dest = malloc(len_a + len_b + len_c + 1);
	strcpy(dest, a);
	strcpy(dest + len_a, b);
	strcpy(dest + len_a + len_b, c);
	return dest;
}

bool str_ends_with(const char *subject, const char *end){
	size_t len_s = strlen(subject), len_e = strlen(end);
	
	// If end is longer than subject it does not work
	if (len_e > len_s)
		return false;
	
	return ( strcmp(subject + (len_s - len_e), end) == 0 );
}


//
// File tree walking functions
//

typedef void (*dir_walker_t)(const char *path, uint8_t type);

/**
 * Traverse a directory tree. Traversal order: directory itself, its files, child directories.
 * This is unlike ftw(). Therefore we had to do it by ourselfs.
 */
void file_tree_walk(const char *path, dir_walker_t walker){
	// Open directory, stop traversing on error
	DIR* dir = opendir(path);
	if (dir == NULL)
		return;
	
	// Directory valid, call waker for current directory
	walker(path, DT_DIR);
	
	// Now walk all files in this directory
	struct dirent *de = NULL;
	while( (de = readdir(dir)) != NULL ){
		if (de->d_type != DT_DIR) {
			char* filepath = str_tricat(path, "/", de->d_name);
			walker(filepath, de->d_type);
			free(filepath);
		}
	}
	
	// Rewind and walk all subdirectories
	rewinddir(dir);
	while( (de = readdir(dir)) != NULL ){
		if (de->d_type == DT_DIR && !(strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0)) {
			char* filepath = str_tricat(path, "/", de->d_name);
			file_tree_walk(filepath, walker);
			free(filepath);
		}
	}
	
	closedir(dir);
}


//
// Image loading functions
//

plains_con_p con;
plains_msg_t msg;
int64_t pos_x = 0;

typedef struct {
	int64_t x, y;
	int w, h;
	size_t level_count;
	uint32_t **levels;
	uint64_t object_id;
	char *filename;
	dir_metadata_p metadata;
} imagedata_t, *imagedata_p;


void create_object(const char* path, dir_metadata_p metadata, void* pixel_data, size_t w, size_t h){
	int smallest_side = (w < h) ? w : h;
	size_t level_count = sizeof(int)*8 - __builtin_clz(smallest_side);
	
	printf("  %zux%zu, %.1f MiByte, %zu levels\n", w, h, (w*h*4.0)/(1024*1024), level_count);
	
	imagedata_p image_data = malloc(sizeof(imagedata_t));
	// the GNU version of basename() does not modify the path, so it's safe
	// to case the const away.
	image_data->filename = strdup(basename((char*)path));
	image_data->metadata = metadata;
	
	data_entry_p mde = metadata_get(metadata, image_data->filename);
	if (mde) {
		image_data->x = mde->x;
		image_data->y = mde->y;
	} else {
		image_data->x = pos_x;
		pos_x += w + 25;
		image_data->y = 0;
	}
	
	image_data->w = w;
	image_data->h = h;
	image_data->level_count = level_count;
	image_data->levels = malloc(level_count * sizeof(image_data->levels[0]));
	image_data->levels[0] = pixel_data;
	
	for(size_t i = 1; i < level_count; i++){
		int lw = w >> i, lh = h >> i;
		image_data->levels[i] = malloc(lw * lh * 4);
		
		// Sizes of the previous level
		int plw = w >> (i-1);
		
		for(size_t ih = 0; ih < lh; ih++){
			for(size_t iw = 0; iw < lw; iw++){
				uint32_t p1 = image_data->levels[i-1][(ih*2    )*plw + iw*2    ];
				uint32_t p2 = image_data->levels[i-1][(ih*2 + 1)*plw + iw*2    ];
				uint32_t p3 = image_data->levels[i-1][(ih*2 + 1)*plw + iw*2 + 1];
				uint32_t p4 = image_data->levels[i-1][(ih*2    )*plw + iw*2 + 1];
				uint32_t pr = 0;
				
				uint8_t* pp1 = (uint8_t*)&p1;
				uint8_t* pp2 = (uint8_t*)&p2;
				uint8_t* pp3 = (uint8_t*)&p3;
				uint8_t* pp4 = (uint8_t*)&p4;
				uint8_t* ppr = (uint8_t*)&pr;
				
				// Loop over each component (rgba)
				for(size_t ic = 0; ic < 4; ic++)
					ppr[ic] = ((uint32_t)pp1[ic] + (uint32_t)pp2[ic] + (uint32_t)pp3[ic] + (uint32_t)pp4[ic]) / 4;
				
				image_data->levels[i][ih*lw + iw] = pr;
				
				/*
				inline uint32_t r(uint32_t rgba){ return  rgba >> 24; }
				inline uint32_t g(uint32_t rgba){ return (rgba >> 16) & 0x000000ff; }
				inline uint32_t b(uint32_t rgba){ return (rgba >> 8)  & 0x000000ff; }
				inline uint32_t a(uint32_t rgba){ return  rgba        & 0x000000ff; }
				inline uint32_t rgba(uint32_t r, uint32_t g, uint32_t b, uint32_t a){
					return (r << 24) & (g << 16) & (b << 8) & a;
				}
				
				
				uint32_t rv = r(p1) + r(p2) + r(p3) + r(p4);
				rv = rv / 4;
				uint32_t gv = g(p1) + g(p2) + g(p3) + g(p4);
				gv = gv / 4;
				uint32_t bv = b(p1) + b(p2) + b(p3) + b(p4);
				bv = bv / 4;
				uint32_t av = a(p1) + a(p2) + a(p3) + a(p4);
				av = av / 4;
				image_data->levels[i][ih*lw + iw] = rgba(rv, gv, bv, av);
				*/
				/*
				image_data->levels[i][ih*lw + iw] = rgba(
					( r(p1) + r(p2) + r(p3) + r(p4) ) / 4,
					( g(p1) + g(p2) + g(p3) + g(p4) ) / 4,
					( b(p1) + b(p2) + b(p3) + b(p4) ) / 4,
					( a(p1) + a(p2) + a(p3) + a(p4) ) / 4
				);
				*/
			}
		}
	}
	
	plains_send(con, msg_object_create(&msg, image_data->x, image_data->y, 0, w, h, image_data));
	plains_msg_print(&msg);
	uint16_t seq = msg.seq;
	
	// Loop until we got the confirmation (and object_id) from the server
	do {
		plains_receive(con, &msg);
		plains_msg_print(&msg);
	} while ( !(msg.type == PLAINS_MSG_STATUS && msg.status.seq == seq) );
	
	image_data->object_id = msg.status.id;
}

void load_file(const char *path, dir_metadata_p metadata){
	printf("loading file %s...\n", path);
	
	int w = 0, h = 0;
	uint32_t* pixel_data = (uint32_t*)stbi_load(path, &w, &h, NULL, 4);
	if (pixel_data) {
		create_object(path, metadata, pixel_data, w, h);
	} else if ( str_ends_with(path, ".pdf") ) {
		uint32_t* pixel_data = NULL;
		size_t page_size = 0;
		
		fz_context *ctx = fz_new_context(NULL, NULL, FZ_STORE_UNLIMITED);
		fz_document *doc = fz_open_document(ctx, strdup(path));
		
		int page_count = fz_count_pages(doc);
		printf("  rendering %d pages\n", page_count);
		
		
		for(size_t page_idx = 0; page_idx < page_count; page_idx++){
			fz_page *page = fz_load_page(doc, page_idx);
			
			fz_rect rect = fz_bound_page(doc, page);
			fz_bbox bbox = fz_round_rect(rect);
			if (pixel_data == NULL) {
				w = bbox.x1 - bbox.x0;
				h = bbox.y1 - bbox.y0;
				page_size = w * h * 4;
				pixel_data = malloc(page_size * page_count);
			}
			
			fz_pixmap *pix = fz_new_pixmap_with_bbox(ctx, fz_device_rgb, bbox);
			fz_clear_pixmap_with_value(ctx, pix, 0xff);
			
			fz_device *dev = fz_new_draw_device(ctx, pix);
			fz_run_page(doc, page, dev, fz_identity, NULL);
			fz_free_device(dev);
			
			if ( fz_pixmap_components(ctx, pix) == 4 ) {
				uint32_t* page_pixel_data = (uint32_t*)fz_pixmap_samples(ctx, pix);
				
				// DO NOT USE page_size for offset calculation. Since we add to a uint32_t pointer
				// the offset is in pixels, not bytes!
				uint32_t* pixel_data_off = pixel_data + w*h*page_idx;
				memcpy(pixel_data_off, page_pixel_data, page_size);
			} else {
				printf("  PDF rendering did not result in RGBA image!\n");
			}
			
			fz_free_page(doc, page);
		}
		
		fz_close_document(doc);
		fz_free_context(ctx);
		
		if (pixel_data)
			create_object(path, metadata, pixel_data, w, h * page_count);
	} else {
		printf("  stbi failed: %s\n", stbi_failure_reason());
	}
	
	//stbi_image_free(data);
}



int main(int argc, char **argv){
	if (argc != 3){
		printf("Usage: %s socket-path dir-or-image\n", argv[0]);
		return 1;
	}
	const char *socket_path = argv[1];
	const char *image_path = argv[2];
	
	con = plains_connect(socket_path);
	
	plains_send(con, msg_hello(&msg, 1, "test client", NULL, 0) );
	//plains_receive(con, &msg);
	//plains_msg_print(&msg);
	
	struct stat s;
	stat(image_path, &s);
	if ( ! S_ISDIR(s.st_mode) ) {
		// Load just the single file
		load_file(image_path, NULL);
	} else {
		dir_metadata_p dir_metadata = NULL;
		// Load dir recursively (ftw traversed directories before their content)
		void walker(const char *path, uint8_t type){
			if (type == DT_DIR) {
				// We're at a directory, look for metadata
				//printf("dir:  %s\n", path);
				dir_metadata = metadata_read(str_append(path, "/.plains"));
			} else if ( type == DT_REG && !str_ends_with(path, ".plains") ) {
				//printf("file: %s\n", path);
				load_file(path, dir_metadata);
			}
		}
		
		file_tree_walk(image_path, walker);
	}
	
	while( plains_receive(con, &msg) > 0 ){
		//plains_msg_print(&msg);
		
		switch(msg.type){
			case PLAINS_MSG_DRAW: {
				imagedata_p img = msg.draw.private;
				// Ignore draw requests without image data
				if (img == NULL)
					break;
				
				size_t shm_size = msg.draw.buffer_width * msg.draw.buffer_height * 4;
				uint32_t *pixel_data = mmap(NULL, shm_size, PROT_WRITE, MAP_SHARED, msg.draw.shm_fd, 0);
				
				
				// Find the closest level
				int level_idx = -log2f(msg.draw.scale);
				// Use level 0 for zoom in
				if (level_idx < 0)
					level_idx = 0;
				
				if (level_idx < img->level_count) {
					// We use a uint32_t pointer since it allows us to handle each pixel as one element
					uint32_t* data = img->levels[level_idx];
					size_t level_width = img->w >> level_idx;
					int64_t level_x = msg.draw.x >> level_idx, level_y = msg.draw.y >> level_idx;
					
					for(size_t by = 0; by < msg.draw.buffer_height; by++){
						size_t oy = level_y + by;
						memcpy(
							pixel_data + by*msg.draw.buffer_width,
							data + oy*level_width + level_x,
							msg.draw.buffer_width * 4
						);
					}
				} else {
					fprintf(stderr, "no image level for scale %f\n", msg.draw.scale);
				}
				
				munmap(pixel_data, shm_size);
				close(msg.draw.shm_fd);
				
				plains_send(con, msg_status(&msg, msg.seq, 0, 0) );
				//plains_msg_print(&msg);
				} break;
			case PLAINS_MSG_MOUSE_MOTION:
				if (msg.mouse_motion.state == 1) {
					imagedata_p img = (imagedata_p)msg.mouse_motion.private;
					// Ignore draw requests without image data
					if (img == NULL)
						break;
					
					img->x += msg.mouse_motion.rel_x;
					img->y += msg.mouse_motion.rel_y;
					plains_send(con, msg_object_update(&msg, img->object_id,
						img->x, img->y, 0, img->w, img->h, img)
					);
					//plains_msg_print(&msg);
					
					metadata_set(img->metadata, img->filename, img->x, img->y);
					metadata_write(img->metadata);
				}
				break;
		}
	}
	
	plains_disconnect(con);
	return 0;
}