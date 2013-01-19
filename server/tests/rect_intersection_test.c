#include <stddef.h>
#include <stdio.h>
#include "../math.h"

int main(){
	irect_t screen = {0, 0, 800, 800};
	irect_t samples[] = {
		// Sample table, structure: object rect to test, expected intersection rect
		// Directly inside
		{  0,   0, 800, 800}, {  0,   0, 800, 800},
		{100, 100,  10,  10}, {100, 100,  10,  10},
		// Part intersection
		{-50,   0, 100, 100}, {  0,   0,  50, 100},
		{750,   0, 100, 100}, {750,   0,  50, 100},
		{  0, -50, 100, 100}, {  0,   0, 100,  50},
		{  0, 750, 100, 100}, {  0, 750, 100,  50},
		// Edge intersections
		{-50, -50, 100, 100}, {  0,   0,  50,  50},
		{750, -50, 100, 100}, {750,   0,  50,  50},
		{750, 750, 100, 100}, {750, 750,  50,  50},
		{-50, 750, 100, 100}, {  0, 750,  50,  50},
		// Touches (width or height of intersection is 0)
		{-100,    0, 100, 100}, {  0,   0,    0,  100},
		// Outside (width or height of intersection is negative)
		{-200,    0, 100, 100}, {  0,   0,    0,  100},
		{ 900,    0, 100, 100}, {900,   0,    0,  100},
		{   0, -200, 100, 100}, {  0,   0,  100,    0},
		{   0,  900, 100, 100}, {  0, 900,  100,    0}
	};
	
	size_t failed_samples = 0;
	for(size_t i = 0; i < sizeof(samples) / 2 / sizeof(irect_t); i++){
		irect_t sampel = samples[i*2], expected = samples[i*2+1];
		irect_t inter = irect_intersection(sampel, screen);
		if (inter.x != expected.x || inter.y != expected.y || inter.w != expected.w || inter.h != expected.h){
			printf("sample %zu failed:\n  sample:   %4ld %4ld %4ld %4ld\n  expected: %4ld %4ld %4ld %4ld\n  inters:   %4ld %4ld %4ld %4ld\n", i,
				sampel.x, sampel.y, sampel.w, sampel.h,
				expected.x, expected.y, expected.w, expected.h,
				inter.x, inter.y, inter.w, inter.h);
			failed_samples++;
		}
	}
	
	printf("%zu of %zu samples failed\n", failed_samples, sizeof(samples) / 2 / sizeof(irect_t));
}