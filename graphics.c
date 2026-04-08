#pragma once

SDL_Rect rect_to_sdl_rect(Rect rect) {
	return (SDL_Rect) {.x=rect.x, .y=rect.y, .w=rect.w, .h=rect.h};
}

void SDL_SetRenderDrawColor32(SDL_Renderer* renderer, uint32_t color) {
	SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
	SDL_SetRenderDrawColor(renderer, (color & 0xFF000000)>>24, (color & 0xFF0000)>>16, (color & 0xFF00 ) >> 8, color & 0xFF);
}

void fill_rect(Rect rect, uint32_t color) {
	SDL_SetRenderDrawColor32(r, color);
	SDL_RenderFillRectF(r, &(SDL_FRect) {.x=rect.x, .y=rect.y, .w=rect.w, .h=rect.h});
}

void draw_rect (Rect rect, uint32_t color) {
	SDL_SetRenderDrawColor32(r, color);
	SDL_RenderDrawRectF(r, &(SDL_FRect) {.x=rect.x, .y=rect.y, .w=rect.w, .h=rect.h});
}

void draw_point(float x, float y, uint32_t color) {
	SDL_SetRenderDrawColor32(r, color);
	SDL_RenderDrawPointF(r, x, y);
}

void draw_line_soft(Arena *arena, double x0, double y0, double x1, double y1, uint32_t color) {
	int n_points_needed_x = ((x1 - x0) > 0) ? (x1 - x0 + 1) : (x0 - x1 + 1);
	int n_points_needed_y = ((y1 - y0) > 0) ? (y1 - y0 + 1) : (y0 - y1 + 1);
	int n_points_needed = (n_points_needed_y > n_points_needed_x) ? n_points_needed_y : n_points_needed_x;
	size_t arena_initial_offset = arena->offset;
	SDL_FPoint *points = arena_get_memory(arena, n_points_needed*sizeof(SDL_FPoint));
	int n_points = 0;

	double slope = (y1 - y0)/(x1 - x0);

	double y, x, dx, dy;

	y = y0;
	x = x0;

	if (x1 == x0) {
		dx = 0;
		dy = (y1 > y0) ? 1 : -1;
	} else if (y1 == y0) {
		dy = 0;
		dx = (x1 > x0) ? 1 : -1;
	} else if ((slope > 1) || (slope < -1)) {
		dy = (y1 > y0) ? 1 : -1;
		dx = 1/slope;
		dx *= dy;
	} else {
		dx = (x1 > x0) ? 1 : -1;
		dy = slope;
		dy *= dx;
	}

end:
	for (int i = 0; i < n_points_needed; i++) {
		points[n_points++] = (SDL_FPoint) {.x=x, .y=y};
		x += dx;
		y += dy;
	}

	SDL_SetRenderDrawColor32(r, color);
	SDL_RenderDrawPointsF(r, points, n_points);
	arena->offset = arena_initial_offset;
}

void draw_line_width(Arena *arena, double x0, double y0, double x1, double y1, uint32_t color, int width) {
	double half_width = width/2.;
	
	double slope = (y1 - y0)/(x1 - x0);
	double orth_slope = -1/slope;

	double y_start_offset, x_start_offset, orth_dx, orth_dy;

	if (x1 == x0) {
		orth_dy = 0;
		orth_dx = 1;
	} else if (y1 == y0) {
		orth_dx = 0;
		orth_dy = 1;
	} else if ((slope > 1) || (slope < -1)) {
		orth_dx = 1;
		orth_dy = orth_slope;
	} else {
		orth_dy = 1;
		orth_dx = -slope;
	}

	x_start_offset = orth_dx * half_width;
	y_start_offset = orth_dy * half_width;
	if (x_start_offset > 0) {
		x_start_offset *= -1;
	}

	if (y_start_offset > 0) {
		y_start_offset *= -1;
	}

	x0 += x_start_offset;
	y0 += y_start_offset;
	x1 += x_start_offset;
	y1 += y_start_offset;

	for (int i = 0; i < width*2; i++) {
		draw_line_soft(arena, x0, y0, x1, y1, color);
		x0 += orth_dx/2;
		x1 += orth_dx/2;
		y0 += orth_dy/2;
		y1 += orth_dy/2;
	}

}

void set_clip_rect(Rect *rect) {
	if (rect == NULL){
		SDL_RenderSetClipRect(r, NULL);
		return;
	}
	SDL_Rect clip_rect = rect_to_sdl_rect(*rect);
	SDL_RenderSetClipRect(r, &clip_rect);
}
//TEXT
int init_font(const char *filename, size_t font_buffer_size, unsigned char font_buffer[font_buffer_size], stbtt_fontinfo *font_info) {
	FILE* font_file = fopen(filename, "rb");
	if (font_file == NULL) {
		printf("Could not open the font file\n");
		return 0;
	}
	fread(font_buffer, 1, font_buffer_size, font_file);
	fclose(font_file);

	if (!stbtt_InitFont(font_info, font_buffer, 0)) {
		fprintf(stderr, "Failed to init the font");
		return 0;
	}

	state.font_info = font_info;
	return 1;
}

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
		break;
		case SDLK_RETURN:
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
