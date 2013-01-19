#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>

#include <ftw.h>

#include <plains/client.h>
#include "stb_image.c"


plains_con_p con;
plains_msg_t msg;
int64_t pos_x = 0;

typedef struct {
	int w, h;
	uint32_t *data;
} imagedata_t, *imagedata_p;

void load_file(const char *path){
	printf("loading file %s...\n", path);
	
	int x = 0, y = 0;
	uint32_t *data = (uint32_t*) stbi_load(path, &x, &y, NULL, 4);
	if (data == NULL){
		printf("  failed: %s\n", stbi_failure_reason());
		return;
	}
	
	printf("  %dx%d, %.1f MiByte\n", x, y, (x*y*4.0)/(1024*1024));
	
	imagedata_p image_data = malloc(sizeof(imagedata_t));
	image_data->w = x;
	image_data->h = y;
	image_data->data = data;
	plains_send(con, msg_object_create(&msg, pos_x, 0, 0, x, y, image_data));
	plains_msg_print(&msg);
	pos_x += x + 25;
	
	plains_receive(con, &msg);
	plains_msg_print(&msg);
	
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
		load_file(image_path);
	} else {
		// Load dir recursively
		int walker(const char *path, const struct stat *sb, int typeflag){
			if (typeflag != FTW_F)
				return 0;
			load_file(path);
			return 0;
		}
		
		ftw(image_path, walker, 10);
	}
	
	while( plains_receive(con, &msg) > 0 ){
		plains_msg_print(&msg);
		/*
		switch(msg.type){
			case MSG_DRAW: {
				size_t shm_size = msg.draw.width * msg.draw.height * 4;
				uint32_t *pixel_data = mmap(NULL, shm_size, PROT_WRITE, MAP_SHARED, msg.fd, 0);
				imagedata_p img = msg.draw.private;
				
				for(size_t j = 0; j < msg.draw.height; j++){
					size_t y = msg.draw.y + j;
					for(size_t i = 0; i < msg.draw.width; i++){
						size_t x = msg.draw.x + i;
						pixel_data[j*msg.draw.width + i] = img->data[y*img->w + x];
					}
				}
				
				munmap(pixel_data, shm_size);
				close(msg.fd);
				
				plains_send(con, msg_status(&msg, msg.seq, 0, 0) );
				plains_msg_print(&msg);
				} break;
		}
		*/
	}
	
	plains_disconnect(con);
	return 0;
}