#pragma once
#include<stdio.h>
#include<stdlib.h>
#include<stdint.h>
#include "arena.c"

int load_ppm(const char *filepath, Arena *arena, int *width_out, int *height_out) {
	FILE *file = fopen(filepath, "r");
	if (file == NULL) {
		fprintf(stderr, "Error reading a file %s at source line %d\n", filepath, __LINE__);
		return 0;
	}
	char magic[2];
	if (fscanf(file, "%c", &magic[0]) != 1) {
		fprintf(stderr, "Error reading a file %s at source line %d\n", filepath, __LINE__);
		return 0;
	}
	if (fscanf(file, "%c", &magic[1]) != 1) {
		fprintf(stderr, "Error reading a file %s at source line %d\n", filepath, __LINE__);
		return 0;
	}

	if (magic[0] != 'P') {
		fprintf(stderr, "Error reading a file %s at source line %d, incorrect format\n", filepath, __LINE__);
		return 0;
	}

	if (magic[1] != '6') {
		fprintf(stderr, "Error reading a file %s at source line %d, incorrect format\n", filepath, __LINE__);
		return 0;
	}

	int width, height;
	if (fscanf(file, "%d", &width) != 1) {
		fprintf(stderr, "Error reading a file %s at source line %d, incorrect format\n", filepath, __LINE__);
		return 0;
	}

	if (fscanf(file, "%d", &height) != 1) {
		fprintf(stderr, "Error reading a file %s at source line %d, incorrect format\n", filepath, __LINE__);
		return 0;
	}

	if (width_out) {
		*width_out = width;
	}
	if (height_out) {
		*height_out = height;
	}

	int depth;
	if (fscanf(file, "%d\n", &depth) != 1) {
		fprintf(stderr, "Error reading a file %s at source line %d, incorrect format\n", filepath, __LINE__);
		return 0;
	}
	if (depth != 255) {
		fprintf(stderr, "Error reading a file %s at source line %d, incorrect format\n", filepath, __LINE__);
		return 0;
	}

	for (int y = 0; y < height; y++) {
		uint32_t *row = arena_get_memory(arena, sizeof(uint32_t)*width);
		for (int x = 0; x < width; x++) {
			uint8_t rgb[3];
			if (fread(rgb, sizeof(uint8_t), 3, file) != 3) {
					fprintf(stderr, "Error reading a file %s at source line %d on iteration x=%d y=%d, incorrect format\n", filepath, __LINE__, x, y);
					return 0;
			}
			row[x] = (0xFF | ((uint32_t)rgb[0] << 24) | ((uint32_t)rgb[1]<<16) | ((uint32_t)rgb[2]<<8));
		}
	}
	return 1;
}
