#pragma once
#include<stdio.h>
#include<stdlib.h>
#include<stdint.h>
#include "arena.c"

typedef enum {
	SVG_COMMAND_MOVETO,
	SVG_COMMAND_LINETO,
	SVG_COMMAND_HORIZONTAL_LINETO,
	SVG_COMMAND_VERTICAL_LINETO,
	SVG_COMMAND_CLOSEPATH,
	SVG_COMMAND_CURVETO,
	SVG_COMMAND_SMOOTH_CURVETO,
	SVG_COMMAND_QUADRATIC_CURVETO,
	SVG_COMMAND_SMOOTH_QUADRATIC_CURVETO,
	SVG_COMMAND_ELIPTICAL_ARC
} SVGCommandKind;

typedef struct {

} SVGCommand;

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

typedef struct {
	uint64_t weight;
	int symbol;
	int left_child;
	int right_child;
	int included_into_tree;
} HuffmanNode;

int compress_huffman(int image_size, uint8_t image_data[static image_size], Arena *scrap_arena) {
	uint64_t *byte_counts = arena_get_memory_zero(scrap_arena, sizeof(byte_counts[0])*256);
	for (int i = 0; i < image_size; i++) {
		uint8_t byte = image_data[i];
		byte_counts[byte]++;
	}

	HuffmanNode *huffman_tree_nodes = arena_get_memory_zero(scrap_arena, sizeof(huffman_tree_nodes[0])*256*2); // There should be at most twice as many nodes as symbols
	int n_huffman_tree_nodes = 0;
	for (int byte = 0; byte < 256; byte++) {
		huffman_tree_nodes[n_huffman_tree_nodes++] = (HuffmanNode) {.weight = byte_counts[byte], .symbol=byte, .left_child=-1, .right_child=-1};
	}

	int enough_nodes = 1;
	int root_node_index = -1;
	while (enough_nodes) {
		int lightest_node_index = -1;
		int lightest_weight = (1<<30);
		for (int i = 0; i < n_huffman_tree_nodes; i++) {
			if (huffman_tree_nodes[i].included_into_tree) {
				continue;
			}
			if (huffman_tree_nodes[i].weight < lightest_weight) {
				lightest_weight = huffman_tree_nodes[i].weight;
				lightest_node_index = i;
			}
		}
		if (lightest_node_index < 0)  {
			fprintf(stderr, "Error when hufmann encoding, ran out of nodes when shouldn't\n");
			return 0;
		}

		HuffmanNode new_node = (HuffmanNode){.weight=lightest_weight, .symbol=-1, .left_child=lightest_node_index};
		huffman_tree_nodes[lightest_node_index].included_into_tree = 1;
		
		lightest_node_index = -1;
		lightest_weight = (1<<30);
		for (int i = 0; i < n_huffman_tree_nodes; i++) {
			if (huffman_tree_nodes[i].included_into_tree) {
				continue;
			}
			if (huffman_tree_nodes[i].weight < lightest_weight) {
				lightest_weight = huffman_tree_nodes[i].weight;
				lightest_node_index = i;
			}
		}
		if (lightest_node_index < 0)  {
			enough_nodes = false;
			root_node_index = new_node.left_child;
			break;
		}
		huffman_tree_nodes[lightest_node_index].included_into_tree = 1;

		new_node.weight += lightest_weight;
		new_node.right_child = lightest_node_index;
		huffman_tree_nodes[n_huffman_tree_nodes++] = new_node;
	}

	printf("%ld\n", huffman_tree_nodes[root_node_index].weight);
}
