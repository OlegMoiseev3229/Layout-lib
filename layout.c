Rect floating_rect(Rect parent, Rect dimention_fractions) { // in dimention_fractions xywh are in range 0.0 to 1.0 and are a percentage of parent rect's dimentions
	return (Rect) {
		.x = parent.w*dimention_fractions.x,
		.y = parent.h*dimention_fractions.y,
		.w = parent.w*dimention_fractions.w,
		.h = parent.h*dimention_fractions.h,
	};
}

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
