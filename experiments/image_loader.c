#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <ftw.h>

#include "stb_image.c"

void load_file(const char *path){
	printf("loading file %s...\n", path);
	
	int x = 0, y = 0;
	uint8_t *data = stbi_load(path, &x, &y, NULL, 4);
	if (data == NULL){
		printf("  failed: %s\n", stbi_failure_reason());
		return;
	}
	
	printf("  %dx%d, %.1f MiByte\n", x, y, (x*y*4.0)/(1024*1024));
	stbi_image_free(data);
}

int main(int argc, char **argv){
	
	if (argc != 2){
		printf("Usage: %s dir-or-image\n", argv[0]);
		return 1;
	}
	
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
	
	return 0;
}