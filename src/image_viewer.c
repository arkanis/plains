#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <ftw.h>

#include "stb_image.c"
#include "libplains.h"


plains_con_p con;
msg_t msg;
int64_t pos_x = 0;

void load_file(const char *path){
	printf("loading file %s...\n", path);
	
	int x = 0, y = 0;
	uint8_t *data = stbi_load(path, &x, &y, NULL, 4);
	if (data == NULL){
		printf("  failed: %s\n", stbi_failure_reason());
		return;
	}
	
	printf("  %dx%d, %.1f MiByte\n", x, y, (x*y*4.0)/(1024*1024));
	plains_send(con, msg_layer_create(&msg, pos_x, 0, 0, x, y, data));
	msg_print(&msg);
	pos_x += x + 25;
	
	plains_receive(con, &msg);
	msg_print(&msg);
	
	//stbi_image_free(data);
}

int main(int argc, char **argv){
	if (argc != 2){
		printf("Usage: %s dir-or-image\n", argv[0]);
		return 1;
	}
	
	con = plains_connect("server.socket");
	
	plains_send(con, msg_hello(&msg, 1, "test client", NULL, 0) );
	plains_receive(con, &msg);
	msg_print(&msg);
	
	struct stat s;
	stat(argv[1], &s);
	if ( ! S_ISDIR(s.st_mode) ) {
		// Load just the single file
		load_file(argv[1]);
	} else {
		// Load dir recursively
		int walker(const char *path, const struct stat *sb, int typeflag){
			if (typeflag != FTW_F)
				return 0;
			load_file(argv[1]);
			return 0;
		}
		
		ftw(argv[1], walker, 10);
	}
	
	while( plains_receive(con, &msg) > 0 ){
		msg_print(&msg);
	}
	
	plains_disconnect(con);
	return 0;
}