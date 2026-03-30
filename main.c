#include <SDL2/SDL.h>
#include <stddef.h>
#include <stdbool.h>
#include <uchar.h>
#include "load_files.c"

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

//TYPES
typedef struct {
	double x, y, w, h;
} Rect;

typedef enum {
	ButtonTypeNone,
	TabButton,
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


//GLOBALS

GlobalState state = {0};

SDL_Renderer* r = {0};
SDL_Window*   w = {0};

Button buttons[1024] = {0};
int n_buttons = 0;
int selected_tab = 1;

Bitmap test_image;

//UTILS
int mouse_clicked_last_frame() {
	static int mouse_pressed_last_frame = 0;
	static int last_frame = 0;

	int mouse_button = SDL_GetMouseState(NULL, NULL);
	int is_mouse_pressed = mouse_button & SDL_BUTTON(1);
	if (is_mouse_pressed && !mouse_pressed_last_frame) {
		if (state.frame != last_frame) {
			mouse_pressed_last_frame = is_mouse_pressed;
			last_frame = state.frame;
		}
		return 1;
	} else {
		if (state.frame != last_frame) {
			mouse_pressed_last_frame = is_mouse_pressed;
			last_frame = state.frame;
		}
		return 0;
	}
}

double clamp(double x, double bottom, double up) {
	if (bottom > up) {
		double temp = bottom;
		bottom = up;
		up = temp;
	}
	if (x < bottom) {
		return bottom;
	}
	if (x > up) {
		return up;
	}
	return x;
}

void print_rect(Rect rect) {
	SDL_Log("x=%f y=%f w=%f h=%f\n", rect.x, rect.y, rect.w, rect.h);
}

int point_in_rect(double x, double y, Rect rect) {
	return ((x < rect.x+rect.w) && (x > rect.x) && (y < rect.y+rect.h) && (y > rect.y));
}

SDL_Rect rect_to_sdl_rect(Rect rect) {
	return (SDL_Rect) {.x=rect.x, .y=rect.y, .w=rect.w, .h=rect.h};
}

int Ustrlen(const char32_t *s) {
	int i;
	for (i = 0; s[i] != U'\0'; i++);
	return i;
}

int utf8_to_utf32(int len, char utf8_str[static len], char32_t utf32_str[static len + 1]) {
	memset(utf32_str, 0, (len + 1)*sizeof(char32_t));
	int utf8_str_index = 0;
	int utf32_str_index = 0;
	while (utf8_str_index < len) {
		unsigned char leading_char = utf8_str[utf8_str_index];
		//SDL_Log("pos %d, decoding %x\n", utf8_str_index, 0xFF & leading_char);
		if ((leading_char & 0x80) == 0) {
			utf32_str[utf32_str_index++] = (leading_char & ~0x80);
			utf8_str_index++;
		} else if ((leading_char & 0xE0) == 0xC0) {
			if (len - utf8_str_index < 1) {
				fprintf(stderr, "Error decoding utf8, not enought bytes at index %d\n", utf8_str_index);
				exit(1);
			}
		 	char32_t new_char = 0;
			new_char |= ((char32_t) (leading_char & ~0xE0)) << (6);
			unsigned char second_char = utf8_str[utf8_str_index + 1];
			if ((second_char & 0xC0) != 0x80) {
				fprintf(stderr, "Error decoding utf8, did not get a continuation character at index %d\n", utf8_str_index);
				exit(1);
			}
			new_char |= ((char32_t) (second_char & ~0xC0));
			utf32_str[utf32_str_index++] = new_char;
			utf8_str_index += 2;
		} else if ((leading_char & 0xF0) == 0xE0) {
			if (len - utf8_str_index < 2) {
				fprintf(stderr, "Error decoding utf8, not enought bytes at index %d\n", utf8_str_index);
				exit(1);
			}
		 	char32_t new_char = 0;
			new_char |= ((char32_t) (leading_char & ~0xF0)) << (6);
			unsigned char second_char = utf8_str[utf8_str_index + 1];
			if ((second_char & 0xC0) != 0x80) {
				fprintf(stderr, "Error decoding utf8, did not get a continuation character at index %d\n", utf8_str_index);
				exit(1);
			}
			new_char |= ((char32_t) (second_char & ~0xC0));
			new_char <<= 6;
			unsigned char third_char = utf8_str[utf8_str_index + 2];
			if ((third_char & 0xC0) != 0x80) {
				fprintf(stderr, "Error decoding utf8, did not get a continuation character at index %d\n", utf8_str_index);
				exit(1);
			}
			new_char |= ((char32_t) (third_char & ~0xC0));
			utf32_str[utf32_str_index++] = new_char;
			utf8_str_index += 3;
		} else if ((leading_char & 0xF8) == 0xF0) {
			exit(1);
		} else if ((leading_char & 0xC0) == 0x80) {
			fprintf(stderr, "Error decoding utf8, unexpected continuation character at index %d\n", utf8_str_index);
			exit(1);
		} else {
			fprintf(stderr, "Error decoding utf8, incorrect character(%x) at index %d\n", 0xFF & leading_char, utf8_str_index);
			exit(1);
		}
	}
	return utf32_str_index;
}


//RENDER
void SDL_SetRenderDrawColor32(SDL_Renderer* renderer, uint32_t color) {
	SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
	SDL_SetRenderDrawColor(renderer, (color & 0xFF000000)>>24, (color & 0xFF0000)>>16, (color & 0xFF00 ) >> 8, color & 0xFF);
}

void fill_rect(Rect rect, uint32_t color) {
	SDL_SetRenderDrawColor32(r, color);
	SDL_RenderFillRectF(r, &(SDL_FRect) {.x=rect.x, .y=rect.y, .w=rect.w, .h=rect.h});
}

void draw_point(float x, float y, uint32_t color) {
	SDL_SetRenderDrawColor32(r, color);
	SDL_RenderDrawPointF(r, x, y);
}

void set_clip_rect(Rect *rect) {
	if (rect == NULL){
		SDL_RenderSetClipRect(r, NULL);
		return;
	}
	SDL_Rect clip_rect = rect_to_sdl_rect(*rect);
	SDL_RenderSetClipRect(r, &clip_rect);
}

//LAYOUT
void rect_divide_vertical(
		Rect parent,
		int n_children,
		Rect *child_rects[n_children],
		double child_weights[static n_children]) {
	double total_weight = 0.;
	for (int i = 0; i < n_children; i++) {
		total_weight += child_weights[i];
	}

	Rect current_child_rect = (Rect) {.x=parent.x, .y=parent.y, .h=parent.h};
	for (int i = 0; i < n_children; i++) {
		current_child_rect.w = (parent.w / total_weight) * child_weights[i];
		if (child_rects[i]) {
			*child_rects[i] = current_child_rect;
		}
		current_child_rect.x += current_child_rect.w;
	}
}

void rect_divide_horizontal(
		Rect parent,
		int n_children,
		Rect *child_rects[n_children],
		double child_weights[static n_children]) {
	double total_weight = 0.;
	for (int i = 0; i < n_children; i++) {
		total_weight += child_weights[i];
	}

	Rect current_child_rect = (Rect) {.x=parent.x, .y=parent.y, .w=parent.w};
	for (int i = 0; i < n_children; i++) {
		current_child_rect.h = (parent.h / total_weight) * child_weights[i];
		if (child_rects[i])
			*child_rects[i] = current_child_rect;
		current_child_rect.y += current_child_rect.h;
	}
}

#define BORDER_SENSITIVITY (10)
void rect_divide_by_border_vertical(
		Rect parent,
		Rect *left_child,
		Rect *right_child,
		double *left_child_size_fraction,
		DraggableBorder *border) {
	border->direction = VerticalBorder;
	int mouse_x, mouse_y;
	int mouse_button = SDL_GetMouseState(&mouse_x, &mouse_y);
	int is_mouse_pressed = mouse_button & SDL_BUTTON(1);
	int is_mouse_clicked = mouse_clicked_last_frame();

	if (!is_mouse_pressed) {
		border->is_dragged = false;
	}
	if (is_mouse_clicked && ((mouse_x - border->x) > - BORDER_SENSITIVITY) && (mouse_x - border->x) < BORDER_SENSITIVITY) {
		border->is_dragged = true;
	}

	if (!border->is_dragged) {
		border->x = parent.w * *left_child_size_fraction;
	} else {
		border->x = mouse_x;
		border->x = clamp(border->x, parent.x, parent.x + parent.w);
		*left_child_size_fraction = (border->x - parent.x)/parent.w;
	}

	rect_divide_vertical(parent, 2, (Rect*[]){left_child, right_child}, (double[]){*left_child_size_fraction, 1-*left_child_size_fraction});
}

Rect draw_area_vertical(Rect parent, double fraction_of_parent_width, uint32_t color) {
	Rect my_rect = {.x=parent.x, .y=parent.y, .w=parent.w*fraction_of_parent_width, .h=parent.h};
	fill_rect(my_rect, color);
	return my_rect;
}

Rect draw_area_horizontal(Rect parent, double fraction_of_parent_height, uint32_t color) {
	Rect my_rect = {.x=parent.x, .y=parent.y, .w=parent.w, .h=parent.h*fraction_of_parent_height};
	fill_rect(my_rect, color);
	return my_rect;
}

void draw_divided_area_vertical(
		Rect parent, int n_children,
	       	Rect* children_rects[static n_children],
	       	double children_weights[static n_children],
	       	uint32_t children_colors[static n_children]) {
	double total_weight = 0.;
	for (int i = 0; i < n_children; i++) {
		total_weight += children_weights[i];
	}
	Rect current_child_rect = (Rect) {.x=parent.x, .y=parent.y, .w=parent.w*(children_weights[0]/total_weight), .h=parent.h};
	for (int i = 0; i < n_children; i++) {
		current_child_rect.w = parent.w*(children_weights[i]/total_weight);
		fill_rect(current_child_rect, children_colors[i]);
		if (children_rects[i]) {
			*children_rects[i] = current_child_rect;
		}
		current_child_rect.x += current_child_rect.w;
	}
}

void draw_divided_area_horizontal(
		Rect parent, int n_children,
	       	Rect* children_rects[static n_children],
	       	double children_weights[static n_children],
	       	uint32_t children_colors[static n_children]) {
	double total_weight = 0.;
	for (int i = 0; i < n_children; i++) {
		total_weight += children_weights[i];
	}
	Rect current_child_rect = (Rect) {.x=parent.x, .y=parent.y, .w=parent.w, .h=parent.h*(children_weights[0]/total_weight)};
	for (int i = 0; i < n_children; i++) {
		current_child_rect.h = parent.h*(children_weights[i]/total_weight);
		fill_rect(current_child_rect, children_colors[i]);
		if (children_rects[i]) {
			*children_rects[i] = current_child_rect;
		}
		current_child_rect.y += current_child_rect.h;
	}
}

//BUTTONS
//Should do before rendering the layout each frame
void clear_buttons() {
	memset(buttons, 0xCD, sizeof(buttons));
	n_buttons = 0;
}


Button get_pressed_button(double mouse_x, double mouse_y) {
	for (int i = 0; i < n_buttons; i++) {
		if (point_in_rect(mouse_x, mouse_y, buttons[i].rect)) {
			return buttons[i];
		}
	}
	return (Button) {.type=ButtonTypeNone};
}

//BORDERS
void snap_child_within_parent_vertical(double* width_fraction, Rect parent, double new_pos) {
	double new_width = new_pos - parent.x;
	new_width = clamp(new_width, 0, parent.w);
	*width_fraction = new_width/parent.w;
}

void draggable_border_drag(DraggableBorder* border, double current_border_pos, double* child_size_fraction, Rect parent) {
	int mouse_x, mouse_y;
	int mouse_button = SDL_GetMouseState(&mouse_x, &mouse_y);
	int is_mouse_pressed = mouse_button & SDL_BUTTON(1);
	int is_mouse_clicked = mouse_clicked_last_frame();
	if (!is_mouse_pressed) {
		border->is_dragged = 0;
		switch (border->direction) {
		case VerticalBorder:
			border->x = current_border_pos;
		break;
		case HorizontalBorder:
			border->y = current_border_pos;
		break;
		}
		return;
	}
	switch (border->direction) {
	case VerticalBorder:
		if (border->is_dragged) {
			border->x = mouse_x;
			snap_child_within_parent_vertical(child_size_fraction, parent, mouse_x);
		} else {
			if (is_mouse_clicked && (mouse_x < border->x + BORDER_SENSITIVITY) && (mouse_x > border->x - BORDER_SENSITIVITY)) {
				border->x = mouse_x;
				border->is_dragged = 1;
			}
		}
	break;
	case HorizontalBorder:
		if (border->is_dragged) {
			border->y = mouse_y;
		} else {
			if (is_mouse_clicked && (mouse_y < border->y + BORDER_SENSITIVITY) && (mouse_y > border->y - BORDER_SENSITIVITY)) {
				border->y = mouse_y;
				border->is_dragged = 1;
			}
		}
	break;
	}
}



void draw_divided_area_with_draggable_border_vertical(
		Rect parent,
	       	Rect* left_child, Rect* right_child,
	       	double* left_child_width_fraction,
	       	uint32_t left_child_color, uint32_t right_child_color,
	       	DraggableBorder* border) {
	border->direction = VerticalBorder;
	Rect left_child_local = {0}; //left_child can be NULL, but I need the data in this function
	draw_divided_area_vertical(parent, 2, (Rect*[]){&left_child_local, right_child}, (double[]){*left_child_width_fraction, 1-*left_child_width_fraction}, (uint32_t[]){left_child_color, right_child_color});
	draggable_border_drag(border, left_child_local.x + left_child_local.w , left_child_width_fraction, parent);
	if (left_child) {
		*left_child = left_child_local;
	}
}

//TEXT

TextDisplay create_editable_text_display(
		const char32_t *initial_text,
	       	stbtt_fontinfo *font,
		float text_size) {
	int text_len = Ustrlen(initial_text);
	char32_t *text = malloc((text_len + 1)*sizeof(char32_t));
	memcpy(text, initial_text, (text_len)*sizeof(char32_t));
	text[text_len] = U'\0';
	return (TextDisplay) {
		.text = text,
		.text_len = text_len,
		.font = font,
		.text_size = text_size,
		.editable = true,
		.cursor_text_position = text_len-1,
	};
}

void draw_text_display(Rect rect, TextDisplay *text_display) {
	//Setting up variables
	int ascent;
	float scale = stbtt_ScaleForPixelHeight(text_display->font, text_display->text_size);
	stbtt_GetFontVMetrics(text_display->font, &ascent,0,0);

	int line_height = ascent * scale;

	int row = 0;
	int current_x = rect.x;

	//Rendering
	set_clip_rect(&rect);

	if (text_display->cursor_text_position == -1) {
		text_display->cursor_position_x = current_x;
		text_display->cursor_position_y = rect.y;
	}
	for (int text_index = 0; text_display->text[text_index] != U'\0'; text_index++) {
		char32_t codepoint = text_display->text[text_index];
		bool dont_draw = false;
		if (codepoint == U'\n') {
			row++;
			current_x = rect.x;
			dont_draw = true;
		}

		int advance;
		stbtt_GetCodepointHMetrics(text_display->font, codepoint, &advance, NULL);
		if (current_x + advance*scale > rect.x + rect.w) {
			current_x = rect.x;
			row++;
		}

		int baseline = line_height * (row + 1);

		if (!dont_draw) {
			int bitmap_width, bitmap_height, baseline_y_offset, current_x_offset;
			unsigned char *bitmap = stbtt_GetCodepointBitmap(
					text_display->font,
					0, scale,
					codepoint,
					&bitmap_width, &bitmap_height, &current_x_offset, &baseline_y_offset
			);

			for (int y_bitmap = 0; y_bitmap < bitmap_height; y_bitmap++) {
				for (int x_bitmap = 0; x_bitmap < bitmap_width; x_bitmap++) {
					uint8_t color = bitmap[x_bitmap + y_bitmap*bitmap_width];
					if (color == 0) {
						continue;
					}
					draw_point( current_x + x_bitmap + current_x_offset,
						       	rect.y + baseline + y_bitmap + baseline_y_offset,
							(0x00000000)|color);
				}
			}
			stbtt_FreeBitmap(bitmap, NULL);
			current_x += advance*scale;
		}
		if (text_index == text_display->cursor_text_position) {
			text_display->cursor_position_x = current_x;
			text_display->cursor_position_y = baseline - line_height + rect.y;
		}
	}

	set_clip_rect(NULL);
	text_display->rect = rect;
	text_display->line_height = line_height;
}

void draw_text_display_cursor(TextDisplay text_display) {
	fill_rect((Rect){.x=text_display.cursor_position_x, .y=text_display.cursor_position_y, .w=2, .h=text_display.line_height}, 0x000000);
}


void focus_if_clicked(GlobalState *state, TextDisplay *text_display) {
	if (state->mouse_clicked && point_in_rect(state->mouse_x, state->mouse_y, text_display->rect)) {
		state->focused_text_display = text_display;
	}
}

void text_display_edit(TextDisplay *text_display, int chars_written, char32_t input_text[static chars_written + 1]) {
	text_display->text = realloc(
			text_display->text, 
			(text_display->text_len + chars_written + 1) * sizeof(char32_t));
	memcpy(text_display->text + text_display->cursor_text_position + chars_written,
		       	text_display->text + text_display->cursor_text_position,
		       	(text_display->text_len - text_display->cursor_text_position)*sizeof(char32_t));
	memcpy(text_display->text + text_display->cursor_text_position+1,
		       	input_text,
		       	chars_written*sizeof(char32_t));
	text_display->text_len += chars_written;
	text_display->cursor_text_position += chars_written;
	text_display->cursor_text_position = clamp(text_display->cursor_text_position, -1, text_display->text_len);
	text_display->text[text_display->text_len] = U'\0';
}
 
void focused_text_input(SDL_Event e) {
	if (!state.focused_text_display) {
		return;
	}
	if (!state.focused_text_display->editable) {
		SDL_Log("Error: trying to edit uneditable text display!\n");
		exit(1);
	}
	switch (e.type) {
	case SDL_KEYDOWN:
		switch (e.key.keysym.sym) {
		case SDLK_BACKSPACE:
			int cursor_pos = state.focused_text_display->cursor_text_position;
			if (cursor_pos < 0) {
				break;
			}
			int text_len = state.focused_text_display->text_len;
			if (text_len <= 0) {
				break;
			}
			for (int i = cursor_pos + 1; i < text_len; i++) {
				state.focused_text_display->text[cursor_pos++] = state.focused_text_display->text[i];
			}
			state.focused_text_display->text[--state.focused_text_display->text_len] = U'\0';
			state.focused_text_display->cursor_text_position--;
			state.focused_text_display->cursor_text_position = clamp(state.focused_text_display->cursor_text_position, -1, state.focused_text_display->text_len);
			//SDL_Log("%d\n", state.focused_text_display->cursor_text_position);
			//SDL_Log("%d\n", state.focused_text_display->text_len);
		break;
		case SDLK_RETURN:
			/*
			state.focused_text_display->text = realloc(
					state.focused_text_display->text, 
					(state.focused_text_display->text_len + 1 + 1) * sizeof(char32_t));
			state.focused_text_display->text[state.focused_text_display->text_len++] = U'\n';
			state.focused_text_display->text[state.focused_text_display->text_len] = U'\0';
			*/
			text_display_edit(state.focused_text_display, 1, U"\n");
		break;
		case SDLK_UP:
		break;
		case SDLK_DOWN:
		break;
		case SDLK_LEFT:
			state.focused_text_display->cursor_text_position--;
			state.focused_text_display->cursor_text_position = clamp(state.focused_text_display->cursor_text_position, -1, state.focused_text_display->text_len);
			
		break;
		case SDLK_RIGHT:
			state.focused_text_display->cursor_text_position++;
			state.focused_text_display->cursor_text_position = clamp(state.focused_text_display->cursor_text_position, -1, state.focused_text_display->text_len);
		break;
		}
	break;
	case SDL_TEXTINPUT:
		char32_t utf32_buffer[SDL_TEXTINPUTEVENT_TEXT_SIZE + 1] = {0};
		int chars_written = utf8_to_utf32(strlen(e.text.text), e.text.text, utf32_buffer);
		/*
		state.focused_text_display->text = realloc(
				state.focused_text_display->text, 
				(state.focused_text_display->text_len + chars_written + 1) * sizeof(char32_t));
		memcpy(state.focused_text_display->text + state.focused_text_display->text_len, utf32_buffer, chars_written*sizeof(char32_t));
		state.focused_text_display->text_len += chars_written;
		state.focused_text_display->cursor_text_position += chars_written;
		state.focused_text_display->cursor_text_position = clamp(state.focused_text_display->cursor_text_position, 0, state.focused_text_display->text_len);
		state.focused_text_display->text[state.focused_text_display->text_len] = U'\0';
		*/
		text_display_edit(state.focused_text_display, chars_written, utf32_buffer);
	break;
	}
}
//IMAGES

void draw_bitmap(Rect parent, Bitmap bitmap) {
	double width_scale = parent.w/bitmap.width;
	double height_scale = parent.h/bitmap.height;
	double scale = width_scale < height_scale ? width_scale : height_scale;

	Rect rect = (Rect){.x = parent.x, .y=parent.y, .w=scale, .h=scale};
	for (int y = 0; y < bitmap.height; ++y) {
		rect.x = parent.x;
		for (int x = 0; x < bitmap.width; ++x) {
			fill_rect(rect, bitmap.pixels[x + y*bitmap.width]);
			rect.x += scale;
		}
		rect.y += scale;
	}
}

//MAIN
void draw_layout(GlobalState *state) {
	clear_buttons();
	int window_width, window_height;
	SDL_GetWindowSize(w, &window_width, &window_height);
	Rect window_rect = (Rect){.w=window_width, .h=window_height};
	fill_rect(window_rect, 0xFFFFFFFF);

	static DraggableBorder border = {0};

	Rect left_area_rect;
	Rect main_area_rect;
	//rect_divide_vertical(window_rect, 2, (Rect*[]){&right_area_rect, &main_area_rect}, (double[]){1., 3.});
	static double left_area_size = 1/4.;
	rect_divide_by_border_vertical(window_rect, &left_area_rect, &main_area_rect, &left_area_size, &border);
	fill_rect(left_area_rect, 0xAAAAAAFF);
	fill_rect(main_area_rect, 0xFFFFFFFF);
	Rect text_area = {0};
	rect_divide_horizontal(left_area_rect, 3, (Rect *[]){NULL, &text_area, NULL}, (double[]){2., 1., 2.});

	static TextDisplay text = {0};
	if (text.font == NULL) text = create_editable_text_display(U"Edit me", state->font_info, 24.);
	draw_text_display(text_area, &text);
	focus_if_clicked(state, &text);
	if (state->focused_text_display == &text) {
		if (state->frame % 60 < 30) {
			draw_text_display_cursor(text);
		}
	}

	static uint32_t pixels[] = {0xFF0000FF, 0x00FF00FF, 0x00FF00FF, 0xFF0000FF};
	draw_bitmap(main_area_rect, test_image);
}

void do_buttons(int mouse_x, int mouse_y) {
	Button pressed_button = get_pressed_button(mouse_x, mouse_y);
	if (pressed_button.type == ButtonTypeNone) {
		return;
	}
	switch (pressed_button.type) {
	case TabButton:
		selected_tab = pressed_button.n;
	break;
	default:
		fprintf(stderr, "Error!: Unknown button type pressed: %d \n", pressed_button.type);
		exit(1);
	}
}

int main() {
	Bitmap bitmap = {.pixels=malloc(1<<20)};
	Arena bitmap_arena = (Arena){.cap=(1<<20), .data=bitmap.pixels, .name="Test bitmap arena"};
	load_ppm("sample.ppm", &bitmap_arena, &bitmap.width, &bitmap.height);
	test_image = bitmap;

	Arena scrap_arena = (Arena){.data=malloc(1<<20), .cap=(1<<20), .name="Scrap arena for compression"};
	compress_huffman(sizeof(test_image.pixels[0])*test_image.width*test_image.height, (void *)test_image.pixels, &scrap_arena);

	SDL_Init(SDL_INIT_EVERYTHING);
	w = SDL_CreateWindow( "test",
				SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
				700, 700.,
				SDL_WINDOW_RESIZABLE
			);
	r = SDL_CreateRenderer(w, -1, 0);
	SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);

	static unsigned char font_data[1<<25];
	FILE* font_file = fopen("./NotoSansJP.ttf", "rb");
	if (font_file == NULL) {
		printf("Could not open the font file\n");
		return -1;
	}
	fread(font_data, 1, 1<<25, font_file);
	fclose(font_file);

	stbtt_fontinfo font_info;
	if (!stbtt_InitFont(&font_info, font_data, 0)) {
		fprintf(stderr, "Failed to init the font");
		return 1;
	}

	state.font_info = &font_info;
	state.frame = 0;

	while (1) {
		SDL_Event e;
		state.mouse_clicked = false;
		state.mouse_x = 0;
		state.mouse_y = 0;
		while (SDL_PollEvent(&e)) {
			switch (e.type) {
			case SDL_QUIT:
				goto end;
			break;
			case SDL_MOUSEBUTTONDOWN:
				do_buttons(e.button.x, e.button.y);
				state.mouse_clicked = true;
				state.mouse_x = e.button.x;
				state.mouse_y = e.button.y;
			break;
			}
			focused_text_input(e);
		}
		draw_layout(&state);
		SDL_RenderPresent(r);
		SDL_Delay(16);
		state.frame++;
	}

end:
	SDL_DestroyRenderer(r);
	SDL_DestroyWindow(w);
	SDL_Quit();
}
