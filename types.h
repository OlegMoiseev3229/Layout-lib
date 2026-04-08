#pragma once
#include "stb_truetype.h"

typedef struct {
	double x, y, w, h;
} Rect;

typedef enum {
	ButtonTypeNone,
	NewSectionButton,
} ButtonType;

typedef struct {
	Rect rect;
	ButtonType type;
	int n;
} Button;

typedef struct {
	char32_t *text;
	stbtt_fontinfo *font;
	int text_len;
	int cursor_position_x;
	int cursor_position_y;
	int cursor_text_position;
	int line_height;
	float text_size;
	Rect rect;
	bool editable;
} TextDisplay;

typedef struct {
	double x;
	double y;
	enum {
		VerticalBorder,
		HorizontalBorder,
	} direction;
	bool is_dragged;
} DraggableBorder;

typedef struct {
	TextDisplay *focused_text_display;
	stbtt_fontinfo *font_info;
	int mouse_x;
	int mouse_y;
	int frame;
	bool mouse_clicked;
} GlobalState;

typedef struct {
	int width;
	int height;
	uint32_t *pixels;
} Bitmap;
