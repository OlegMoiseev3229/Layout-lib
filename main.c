#include <SDL2/SDL.h>
#include <stddef.h>
#include <stdbool.h>
#include <uchar.h>

#include "types.h"
#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"


//GLOBALS
GlobalState state = {0};

SDL_Renderer* r = {0};
SDL_Window*   w = {0};

Button buttons[1024] = {0};
int n_buttons = 0;
int n_sections = 1;

#include "utils.c"
#include "load_files.c"
#include "layout.c"
#include "graphics.c"


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

//MAIN
void draw_frame_counter(Rect parent, GlobalState *state) {
	static char32_t text[11] = {0};
	static TextDisplay frame_display = {0};
	if (frame_display.font == NULL) {
		frame_display = (TextDisplay){.font=state->font_info, .text_size=32., .text=text};
	}
	int2utf32(state->frame, frame_display.text);
	Rect rect = floating_rect(parent, (Rect){.x=0.9, .y=0.9, .w=0.1, .h=0.05});
	draw_text_display(rect, &frame_display);
}

void draw_layout(GlobalState *state) {
	clear_buttons();
	int window_width, window_height;
	SDL_GetWindowSize(w, &window_width, &window_height);
	Arena graphics_arena = (Arena) {.cap=(1<<20), .data=malloc(1<<20), .name="Graphics arena"};
	Rect window_rect = (Rect){.w=window_width, .h=window_height};
	fill_rect(window_rect, 0xFFFFFFFF);

	draw_line_width(&graphics_arena, 400, 100, 500, 700, 0xFF0000FF, 10);
	if (n_sections > 5) {
		n_sections = 1;
	}

	Rect section_rects[5] = {0};
	Rect *section_rect_pointers[5] = {
		&section_rects[0],
		&section_rects[1],
		&section_rects[2],
		&section_rects[3],
		&section_rects[4],
	};
	double section_rect_weights[5] = {1, 1, 1, 1, 1};

	rect_divide_vertical(window_rect, n_sections, section_rect_pointers, section_rect_weights);

	static TextDisplay test_text_display = {0};
       	if (test_text_display.font == NULL) {
		test_text_display = (TextDisplay) {.text=U"Some sample text, I just need to write whatever in here to fill some space so that there is some wrapping or whatever and I can clearly see the different things or whatever they are called........... sections, right, so that I can see sections, different sections. I hope I wrote enough because I'm running out of Ideas on what to write here. I mean I'm already writing about how I ran out of ideas, this is the dumbest topic to write about. Maybe there are dumber topics. I can't come up with any more dumb topics, though. \nIthought that it would be cool to throw in a new line character in here to check that nothing broke since I wrote the code a couple of weeks ago. I haven't changed anything but who knows", .font=state->font_info, .text_size=32.};
	}
	for (int i = 0; i < n_sections; i++) {
		Rect upper_part;
		Rect bottom_part;
		rect_divide_horizontal(section_rects[i], 2, (Rect*[]){&upper_part, &bottom_part}, (double[]){3., 1.});
		fill_rect(bottom_part, 0xBBBBBBFF);
		draw_text_display(upper_part, &test_text_display);
		draw_rect(section_rects[i], 0x000000FF);
	}

	Rect new_section_button = floating_rect(window_rect, (Rect){.x=0.05, .y=0.05, .w=0.17, .h=0.05});
	fill_rect(new_section_button, 0x555555FF);

	static TextDisplay new_section_text_display = {0};
       	if (new_section_text_display.font == NULL) {
		new_section_text_display = (TextDisplay) {.text=U"New section", .font=state->font_info, .text_size=32.};
	}
	buttons[n_buttons++] = (Button){.rect=new_section_button, .type=NewSectionButton};

	draw_text_display(new_section_button, &new_section_text_display);

	draw_frame_counter(window_rect, state);
	free(graphics_arena.data);
}

void do_buttons(int mouse_x, int mouse_y) {
	Button pressed_button = get_pressed_button(mouse_x, mouse_y);
	if (pressed_button.type == ButtonTypeNone) {
		return;
	}
	switch (pressed_button.type) {
	case NewSectionButton:
		n_sections++;
	break;
	default:
		fprintf(stderr, "Error!: Unknown button type pressed: %d \n", pressed_button.type);
		exit(1);
	}
}

int main() {
	SDL_Init(SDL_INIT_EVERYTHING);
	w = SDL_CreateWindow( "test",
				SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
				700, 700.,
				SDL_WINDOW_RESIZABLE
			);
	r = SDL_CreateRenderer(w, -1, 0);

	static unsigned char font_data[1<<25];
	stbtt_fontinfo font_info;
	if (!init_font("./NotoSansJP.ttf", 1<<25, font_data, &font_info)) {
		return 1;
	}
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
