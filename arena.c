#pragma once
#include<stdio.h>

typedef struct {
	void *data;
	size_t offset;
	size_t cap;
} Arena;

void *arena_get_memory(Arena *arena, size_t bytes) {
	if (arena->cap - arena->offset < bytes) {
		fprintf(stderr, "Out of memory on the arena\n");
		return NULL;
	}
	void *res = (uint8_t*)arena->data + arena->offset;
	memset(res, 0xCE, bytes);
	arena->offset += bytes;
	return res;
}

void *arena_get_memory_zero(Arena *arena, size_t bytes) {
	if (arena->cap - arena->offset < bytes) {
		fprintf(stderr, "Out of memory on the arena\n");
		return NULL;
	}
	void *res = (uint8_t*)arena->data + arena->offset;
	memset(res, 0x00, bytes);
	arena->offset += bytes;
	return res;
}

void *arena_get_memory_neg(Arena *arena, size_t bytes) {
	if (arena->cap - arena->offset < bytes) {
		fprintf(stderr, "Out of memory on the arena\n");
		return NULL;
	}
	void *res = (uint8_t*)arena->data + arena->offset;
	memset(res, 0xFF, bytes);
	arena->offset += bytes;
	return res;
}
