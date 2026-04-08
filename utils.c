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

void int2utf32(uint32_t i, char32_t text_out[11]) {
	static char32_t text_temp[11] = {0};
	memset(text_out, 0, sizeof(text_out[0])*11);
	memset(text_temp, 0, sizeof(text_temp));

	int j = 0;
	do {
		char32_t c = (i % 10) + U'0'; 
		i /= 10;
		text_temp[j++] = c;
	} while (i > 0);

	for (int k = 0; k < j; k++) {
		text_out[k] = text_temp[j - k - 1];
	}
}
