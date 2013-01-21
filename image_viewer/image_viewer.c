#include <stdint.h>
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
#include "stb_image.c"


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

char* str_append(const char *a, const char *b){
	size_t alen = strlen(a), blen = strlen(b);
	char* dest = malloc(alen + blen + 1);
	strcpy(dest, a);
	strcpy(dest + alen, b);
	return dest;
}



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

void load_file(const char *path, dir_metadata_p metadata){
	printf("loading file %s...\n", path);
	
	int w = 0, h = 0;
	uint32_t *data = (uint32_t*)stbi_load(path, &w, &h, NULL, 4);
	if (data == NULL){
		printf("  failed: %s\n", stbi_failure_reason());
		return;
	}
	
	int smallest_side = (w < h) ? w : h;
	size_t level_count = sizeof(int)*8 - __builtin_clz(smallest_side);
	
	printf("  %dx%d, %.1f MiByte, %zu levels\n", w, h, (w*h*4.0)/(1024*1024), level_count);
	
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
	image_data->levels[0] = data;
	
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
	
	//stbi_image_free(data);
}


typedef void (*dir_walker_t)(const char *path, uint8_t type);

/**
 * Traverse a directory tree. Traversal order: directory itself, its files, child directories.
 * This is unlike ftw(). Therefore we had to do it by ourselfs.
 */
void file_tree_walk(const char *path, dir_walker_t walker){
	// Walk our directory itself
	walker(path, DT_DIR);
	
	char* old_wd = getcwd(NULL, 0); // Buffer allocated automatically (GNU ext.)
	
	DIR* dir = opendir(path);
	struct dirent *de = NULL;
	chdir(path);
	
	// Now walk all files in this directory
	while( (de = readdir(dir)) != NULL ){
		if (de->d_type != DT_DIR)
			walker(de->d_name, de->d_type);
	}
	
	// Rewind and walk all subdirectories
	rewinddir(dir);
	while( (de = readdir(dir)) != NULL ){
		if (de->d_type == DT_DIR && !(strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0))
			file_tree_walk(de->d_name, walker);
	}
	
	closedir(dir);
	
	chdir(old_wd);
	free(old_wd);
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
				dir_metadata = metadata_read(str_append(path, "/.plains"));
			} else if (type == DT_REG) {
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