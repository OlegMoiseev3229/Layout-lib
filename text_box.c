#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <uchar.h>
#include <math.h>
#include <SDL2/SDL.h>



#if 0
int main() {
	SDL_Init(SDL_INIT_EVERYTHING);
	SDL_Window* w = SDL_CreateWindow("font-test", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 700, 700, 0);
	r = SDL_CreateRenderer(w, -1, 0);

	//FILE* font_file = fopen("./LiberationMono-Regular.ttf", "rb");
	FILE* font_file = fopen("./NotoSansJP.ttf", "rb");
	if (font_file == NULL) {
		printf("Could not open the font file\n");
		return -1;
	}
	int res = fread(font_data, 1, 1<<25, font_file);
	fclose(font_file);

	stbtt_fontinfo font_info;
	if (!stbtt_InitFont(&font_info, font_data, 0)) {
		fprintf(stderr, "Failed to init the font");
		return 1;
	}

	GlobalState state = {0};
	int frame = 0;
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
				state.mouse_clicked = true;
				state.mouse_x = e.button.x;
				state.mouse_y = e.button.y;
			break;
			}
			focused_text_input(e, state);
		}

		SDL_Rect rect;
		SDL_SetRenderDrawColor(r, 0xFF, 0xFF, 0xFF, 0xFF);
		SDL_RenderFillRect(r, NULL);

		TextDisplay text = {.text=U"Hello world!\nUwU", .font = &font_info, .text_size = 64.};
		draw_text_display(r, (SDL_Rect){.x=100, .y=100, .h=500, .w=100}, &text);

		TextDisplay text2 = {.text=U"owo", .font = &font_info, .text_size = 32.};
		draw_text_display(r, (SDL_Rect){.x=500, .y=100, .h=100, .w=100}, &text2);

		static TextDisplay text3 = {0};
		if (!text3.font) text3 = create_editable_text_display(U"Hi there,\nhello!", &font_info, 24.);

		rect = (SDL_Rect){.x=300, .y=400, .h=200, .w=200};
		if (state.focused_text_display == &text3) {
			SDL_SetRenderDrawColor(r, 0xD0, 0xD0, 0xD0, 0xFF);
			SDL_RenderDrawRect(r, &rect);
			if (frame % 60 < 30) {
				draw_text_display_cursor(text3);
			}
		}
		draw_text_display(r, rect, &text3);
		focus_if_clicked(&state, &text3);

		static TextDisplay text4 = {0};
		if (!text4.font) text4 = create_editable_text_display(U"Not hi there,\ngoodbye", &font_info, 24.);
		
		rect = (SDL_Rect){.x=300, .y=100, .h=200, .w=200};

		if (state.focused_text_display == &text4) {
			SDL_SetRenderDrawColor(r, 0xD0, 0xD0, 0xD0, 0xFF);
			SDL_RenderDrawRect(r, &rect);
			if (frame % 60 < 30) {
				draw_text_display_cursor(text4);
			}
		}
		draw_text_display(r, rect, &text4);
		focus_if_clicked(&state, &text4);

		SDL_RenderPresent(r);
		SDL_Delay(16);
		frame++;
	}
end: 
	SDL_Quit();
}
#endif
